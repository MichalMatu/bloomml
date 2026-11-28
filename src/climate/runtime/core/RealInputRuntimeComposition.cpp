#include "climate/runtime/core/RealInputRuntimeComposition.h"

#include "climate/output/OutputBindings.h"
#include "climate/runtime/RuntimeBuildConfig.h"

#include <nvs_flash.h>

#include <array>

namespace growbox::app::climate_io::runtime {
namespace {

storage::Stage27TelemetryLogger::Config makeStorageConfig() noexcept {
  storage::Stage27TelemetryLogger::Config config{};
  config.sd_pins = {runtime_config::kSdMosiGpio, runtime_config::kSdMisoGpio,
                    runtime_config::kSdSclkGpio, runtime_config::kSdCsGpio,
                    runtime_config::kSdPowerGpio};
  config.sd_enabled = runtime_config::kStage27SdEnabled;
  config.flash_fallback_enabled = runtime_config::kStage27FlashFallbackEnabled;
  config.sd_cmd0_precondition = runtime_config::kSdCmd0Precondition;
  return config;
}

RfDiagnosticsConfig makeRfDiagnosticsConfig() noexcept {
  RfDiagnosticsConfig config{};
  config.enabled = runtime_config::kRf433LoopbackEnabled;
  config.passive_capture = runtime_config::kRf433RemoteCaptureEnabled;
  config.tx_gpio = runtime_config::kRf433TxGpio;
  config.rx_gpio = runtime_config::kRf433RxGpio;
  return config;
}


} // namespace

bool RuntimeNvsOwner::begin() noexcept {
  if (initialized_) {
    return ready_;
  }

  initialized_ = true;
  esp_err_t error = nvs_flash_init();
  if (error == ESP_ERR_NVS_NO_FREE_PAGES || error == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    const esp_err_t erase_error = nvs_flash_erase();
    if (erase_error != ESP_OK) {
      last_error_ = static_cast<std::int32_t>(erase_error);
      return false;
    }
    erased_on_recovery_ = true;
    error = nvs_flash_init();
  }

  last_error_ = static_cast<std::int32_t>(error);
  ready_ = error == ESP_OK;
  return ready_;
}

RuntimeIoOwner::RuntimeIoOwner() noexcept
    : storage_config_(makeStorageConfig()), storage_logger_(storage_config_),
      rf_diagnostics_config_(makeRfDiagnosticsConfig()),
      rf_radio_(rf433::Rf433RmtLoopback::Config{rf_diagnostics_config_.tx_gpio,
                                                rf_diagnostics_config_.rx_gpio}),
      rf_diagnostics_(rf_diagnostics_config_, rf_radio_), rf_frame_sender_(rf_radio_),
      rf_output_transport_(rf_frame_sender_) {}

bool RuntimeIoOwner::beginRf() noexcept {
  const bool radio_ready = rf_diagnostics_config_.enabled && rf_radio_.begin();
  return rf_diagnostics_.begin(radio_ready);
}

output::ConfiguredOutputEquipment::Bindings makeMintOutputAdapters(
    output::OutputTransport& transport) noexcept {
  using ::growbox::ipo::Driver;
  return {{{Driver::Rf433BinarySocket, 1U, rf433::kRemoteSocket1ClimateEndpoint,
            rf433::kRemoteSocket1ClimateEndpoint, &transport},
           {Driver::Rf433BinarySocket, 2U, rf433::kRemoteSocket2ClimateEndpoint,
            rf433::kRemoteSocket2ClimateEndpoint, &transport},
           {Driver::Rf433BinarySocket, 3U, rf433::kRemoteSocket3ClimateEndpoint,
            rf433::kRemoteSocket3ClimateEndpoint, &transport}}};
}

RuntimePersistenceOwner::RuntimePersistenceOwner(
    const output::ConfiguredOutputEquipment& equipment) noexcept
    : equipment_(equipment), persistence_store_(nvs_backend_, equipment.policy()),
      persistence_(persistence_store_) {}

void RuntimePersistenceOwner::initialize(bool persistent_storage_ready) noexcept {
  std::array<output::OutputEndpointId, output::kOutputEndpointCapacity> endpoints{};
  const auto& policy = equipment_.policy();
  for (std::size_t i = 0; i < policy.count; ++i) endpoints[i] = policy.endpoints[i].endpoint;
  state_store_ready_ = equipment_.valid() && state_store_.configure(endpoints, policy.count);
  init_result_ = state_store_ready_ && persistent_storage_ready
                     ? persistence_.initialize(state_store_)
                     : output::OutputPersistenceCoordinatorInitResult{};
}

const output::OutputPolicyConfig& RuntimePersistenceOwner::policy() const noexcept {
  return persistence_.valid() ? persistence_.policy() : equipment_.policy();
}

RuntimeOutputOwner::RuntimeOutputOwner(const output::ConfiguredOutputEquipment& equipment,
                                       RuntimeExecutionStatus& execution_status,
                                       RfDiagnostics& diagnostics,
                                       const output::OutputPolicyConfig& policy,
                                       output::OutputStateStore& state_store) noexcept
    : policy_(policy), semantic_output_config_(equipment.semantic()),
      binary_policies_(equipment), configured_transport_(equipment),
      supervisor_config_(binary_policies_.config()),
      supervisor_transport_(configured_transport_, execution_status), output_lifecycle_(policy_),
      lifecycle_executor_(policy_, output_lifecycle_, supervisor_transport_, state_store,
                          supervisor_config_),
      runtime_lifecycle_(output_lifecycle_, lifecycle_executor_),
      automation_control_(output_lifecycle_, lifecycle_executor_),
      manual_control_(policy_, output_lifecycle_), maintenance_rf_transport_(diagnostics),
      maintenance_control_(policy_, output_lifecycle_, automation_control_, lifecycle_executor_,
                           state_store, supervisor_config_, maintenance_rf_transport_),
      supervisor_resolver_(supervisor_config_),
      supervisor_executor_(supervisor_transport_, state_store, supervisor_config_),
      supervisor_sink_(semantic_output_config_, supervisor_resolver_, supervisor_executor_,
                       state_store) {
  bindings_valid_ = equipment.valid() &&
      output::validateOutputPolicyConfig(policy_) == output::OutputPolicyConfigStatus::Ok &&
      policy_.count == equipment.policy().count;
  for (std::size_t i = 0; bindings_valid_ && i < policy_.count; ++i) {
    const auto& ep = policy_.endpoints[i];
    const auto* expected = output::findOutputPolicyRole(equipment.policy(), ep.role);
    bindings_valid_ = expected && expected->endpoint == ep.endpoint;
  }
  valid_ = bindings_valid_ && output_lifecycle_.valid() && lifecycle_executor_.valid() &&
           runtime_lifecycle_.valid() && automation_control_.valid() && manual_control_.valid() &&
           maintenance_control_.valid() && supervisor_sink_.valid();
}

RuntimeControlOwner::RuntimeControlOwner() noexcept
    : runtime_controller_(nullptr, productionRuntimeConfig()) {}

} // namespace growbox::app::climate_io::runtime
