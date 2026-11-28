#include "climate/output/supervisor/ClimateOutputSupervisorSink.h"
#include "climate/output/ConfiguredOutputEquipment.h"
#include "climate/output/ConfiguredOutputPolicies.h"
#include "IpoBoardAdmission.h"
#include "climate/application/ClimateInputContracts.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace {

namespace climate_io = growbox::app::climate_io;
namespace climate = growbox::climate;
namespace output = growbox::app::output;

constexpr output::OutputEndpointId kFan = 1U;
constexpr output::OutputEndpointId kLamp = 2U;
constexpr output::OutputEndpointId kHumidifier = 3U;

class FakeTransport final : public output::OutputTransport {
public:
  std::array<output::TxResult, 16U> scripted{};
  std::array<output::OutputCommand, 16U> sent{};
  std::size_t scripted_count{0U};
  std::size_t scripted_index{0U};
  std::size_t sent_count{0U};

  output::TxResult send(const output::OutputCommand& command) noexcept override {
    assert(sent_count < sent.size());
    sent[sent_count++] = command;
    if (scripted_index < scripted_count) {
      return scripted[scripted_index++];
    }
    return {output::TransportStatus::Completed, output::TransportError::None};
  }
};

class DualPortFakeDevice final : public output::OutputTransport,
                                 public climate_io::DeviceElectricalSource {
public:
  output::TxResult send(const output::OutputCommand& command) noexcept override {
    last_endpoint = command.endpoint;
    ++writes;
    return {output::TransportStatus::Completed, output::TransportError::None};
  }
  bool sampleElectrical(std::uint64_t, climate_io::DeviceElectricalSnapshot& value) noexcept override {
    value = {42U, {12.0F, true, 0U}, {150.0F, true, 0U}};
    ++reads;
    return true;
  }
  output::OutputEndpointId last_endpoint{};
  unsigned writes{0U}, reads{0U};
};

output::BinaryActuatorPolicy makePolicy(std::uint64_t min_on_ms = 0U,
                                        std::uint64_t min_off_ms = 0U) {
  output::BinaryActuatorPolicyConfig config{};
  config.on_threshold = 0.10F;
  config.off_threshold = 0.03F;
  config.min_on_ms = min_on_ms;
  config.min_off_ms = min_off_ms;
  return output::BinaryActuatorPolicy(config);
}

output::OutputStateStore makeStore() {
  output::OutputStateStore store;
  const std::array<output::OutputEndpointId, output::kOutputEndpointCapacity> endpoints{
      kFan, kLamp, kHumidifier};
  assert(store.configure(endpoints, 3U));
  return store;
}

output::OutputSupervisorResolverConfig
makeSupervisorConfig(output::BinaryActuatorPolicy& fan, output::BinaryActuatorPolicy& humidifier) {
  output::OutputSupervisorResolverConfig config{};
  config.endpoints[0] = {kFan, &fan};
  config.endpoints[1] = {kLamp, nullptr};
  config.endpoints[2] = {kHumidifier, &humidifier};
  config.count = 3U;
  return config;
}

climate_io::ClimateSemanticOutputConfig makeClimateConfig() {
  climate_io::ClimateSemanticOutputConfig config{};
  assert(climate_io::bindClimateRole(config, climate_io::ClimateActuatorRole::ExhaustFan, kFan));
  assert(climate_io::bindClimateRole(config, climate_io::ClimateActuatorRole::Humidifier,
                                     kHumidifier));
  assert(climate_io::validateClimateSemanticOutputConfig(config) ==
         climate_io::ClimateSemanticOutputConfigStatus::Ok);
  return config;
}

climate_io::ClimateOutputSupervisorCycleContext scheduleContext(bool lamp_on) {
  climate_io::ClimateOutputSupervisorCycleContext context{};
  context.schedule.metadata.sequence = 70U;
  context.schedule.metadata.source = output::OutputSource::Schedule;
  context.schedule.metadata.reason = output::OutputReason::ScheduleRequest;
  assert(output::setEndpointIntent(context.schedule.endpoints[0], kLamp, lamp_on ? 1.0F : 0.0F));
  return context;
}

climate::ClimatePolicyRequest request(float fan, float humidifier) {
  climate::ClimatePolicyRequest value{};
  value.exhaust_fan = fan;
  value.humidifier = humidifier;
  return value;
}

bool near(float left, float right) {
  return std::fabs(left - right) <= 1.0e-6F;
}

void assertPhysicalUnknown(const output::OutputStateStore& store,
                           output::OutputEndpointId endpoint) {
  const auto* state = store.find(endpoint);
  assert(state != nullptr);
  assert(state->physical.state == output::PhysicalOutputState::Unknown);
  assert(!state->physical.has_independent_feedback);
}

void testMintEquipmentParity() {
  namespace ipo = growbox::ipo;
  FakeTransport adapter;
  const auto graph = ipo::makeCrowPanelMintReferenceGraph();
  output::ConfiguredOutputEquipment::Bindings bindings{};
  for (std::uint16_t i = 1; i <= 3; ++i)
    bindings[i-1U] = {ipo::Driver::Rf433BinarySocket, i, i, i, &adapter};
  output::ConfiguredOutputEquipment equipment(graph, bindings, 3U);
  assert(equipment.valid());
  const auto previous = output::makeSafeDefaultOutputPolicyConfig(1U, 2U, 3U);
  const auto& next = equipment.policy();
  assert(previous.count == next.count && previous.version == next.version);
  assert(previous.max_transition_failures == next.max_transition_failures);
  for (std::size_t i = 0; i < next.count; ++i) {
    const auto& a = previous.endpoints[i];
    const auto& b = next.endpoints[i];
    assert(a.endpoint == b.endpoint && a.role == b.role);
    for (std::size_t j = 0; j < a.lifecycle.size(); ++j) {
      const auto& x = a.lifecycle[j]; const auto& y = b.lifecycle[j];
      assert(x.action == y.action && x.order == y.order && x.delay_ms == y.delay_ms);
      assert(x.retransmit == y.retransmit && x.max_retries == y.max_retries);
    }
  }
  output::ConfiguredOutputPolicies policies(equipment);
  const auto& config = policies.config();
  assert(config.endpoints[0].endpoint == 2U && !config.endpoints[0].binary_policy);
  assert(config.endpoints[1].endpoint == 1U);
  assert(config.endpoints[1].binary_policy->config().min_on_ms == 120000U);
  assert(config.endpoints[1].binary_policy->config().min_off_ms == 120000U);
  assert(config.endpoints[2].endpoint == 3U);
  assert(config.endpoints[2].binary_policy->config().min_on_ms == 180000U);
  assert(config.endpoints[2].binary_policy->config().min_off_ms == 180000U);
  const auto caps = equipment.capabilities();
  assert(caps.exhaust_fan && caps.humidifier && !caps.heater && !caps.cooler &&
         !caps.dehumidifier && !caps.co2_doser);
  assert(!ipo::assessCrowPanelReference(graph).apply_authorized);
}

void testConfiguredSupervisorAllRoles() {
  namespace ipo = growbox::ipo;
  ipo::Graph graph{};
  graph.processes[graph.process_count++] = {1U, ipo::ProcessKind::Co2Safety, true};
  FakeTransport adapter;
  output::ConfiguredOutputEquipment::Bindings bindings{};
  std::array<output::OutputEndpointId, output::kOutputEndpointCapacity> endpoints{};
  for (unsigned role = 0; role < 7U; ++role) {
    const auto id = static_cast<std::uint16_t>(role+1U);
    graph.devices[graph.device_count++] = {id, ipo::Driver::GpioRelay, id, true};
    graph.actuators[graph.actuator_count++] = {
        static_cast<std::uint16_t>(100U+id), id, ipo::Channel::Switch,
        static_cast<ipo::OutputRole>(role), true, 1000U, 2000U};
    endpoints[role] = static_cast<output::OutputEndpointId>(10U+id);
    bindings[role] = {ipo::Driver::GpioRelay, id, endpoints[role], id, &adapter};
  }
  output::ConfiguredOutputEquipment equipment(graph, bindings, 7U);
  assert(equipment.valid());
  output::ConfiguredOutputPolicies policies(equipment);
  assert(policies.config().count == 7U);
  output::OutputStateStore store;
  assert(store.configure(endpoints, 7U));
  output::ConfiguredOutputTransport transport(equipment);
  output::OutputSupervisorResolver resolver(policies.config());
  output::OutputSupervisorExecutor executor(transport, store, policies.config());
  climate_io::ClimateOutputSupervisorSink sink(equipment.semantic(), resolver, executor, store);
  climate_io::ClimateOutputSupervisorCycleContext context{};
  assert(output::setEndpointIntent(context.schedule.endpoints[0], endpoints[2], 1.0F));
  sink.setCycleContext(context);
  climate::ClimatePolicyRequest demand{};
  demand.heater = demand.cooler = demand.exhaust_fan = demand.humidifier =
      demand.dehumidifier = demand.co2_doser = 1.0F;
  // This is the Output contract only, not the Rule arbitration policy.
  climate::ClimateExecutionProjection execution{};
  assert(sink.applyAndReportExecution(demand, 1000U, execution));
  assert(adapter.sent_count == 7U);
  for (auto ep : endpoints) assertPhysicalUnknown(store, ep);
  assert(execution.known_mask == climate::ClimateExecutionKnownAll);
  assert(sink.applyAndReportExecution({}, 1500U, execution));
  assert(adapter.sent_count == 7U); // dwell prevents early OFF.
}

void testConfiguredEquipmentAndTransportSubstitution() {
  namespace ipo = growbox::ipo;
  for (unsigned mask = 0; mask < 128U; ++mask) {
    ipo::Graph graph{};
    graph.processes[graph.process_count++] = {1U, ipo::ProcessKind::Co2Safety, true};
    FakeTransport first, second;
    output::ConfiguredOutputEquipment::Bindings bindings{};
    for (unsigned role = 0; role < 7U; ++role) {
      if (!(mask & (1U << role))) continue;
      const auto device = static_cast<std::uint16_t>(role + 1U);
      graph.devices[graph.device_count++] = {device, ipo::Driver::GpioRelay, device, true};
      graph.actuators[graph.actuator_count++] = {
          static_cast<std::uint16_t>(100U+device), device, ipo::Channel::Switch,
          static_cast<ipo::OutputRole>(role), true, 1000U, 2000U};
      bindings[graph.actuator_count-1U] = {ipo::Driver::GpioRelay, device,
          static_cast<output::OutputEndpointId>(10U+device), device, &first};
    }
    output::ConfiguredOutputEquipment equipment(graph, bindings, graph.actuator_count);
    if (!mask) {
      assert(!equipment.valid() && equipment.status() == output::EquipmentStatus::Empty);
      continue;
    }
    assert(equipment.valid() && equipment.policy().count == graph.actuator_count);
    const auto caps = equipment.capabilities();
    assert(caps.exhaust_fan == bool(mask & 1U));
    assert(caps.humidifier == bool(mask & 2U));
    assert(caps.heater == bool(mask & 8U));
    assert(caps.cooler == bool(mask & 16U));
    assert(caps.dehumidifier == bool(mask & 32U));
    assert(caps.co2_doser == bool(mask & 64U));
    output::ConfiguredOutputTransport transport(equipment);
    output::OutputCommand command{};
    command.endpoint = bindings[0].endpoint;
    command.state = output::BinaryOutputState::On;
    assert(transport.send(command).status == output::TransportStatus::Completed);
    assert(first.sent_count == 1U && first.sent[0].endpoint == bindings[0].adapter_endpoint);
    bindings[0].adapter = &second;
    output::ConfiguredOutputEquipment replacement(graph, bindings, graph.actuator_count);
    output::ConfiguredOutputTransport other(replacement);
    assert(other.send(command).status == output::TransportStatus::Completed);
    assert(second.sent_count == 1U && first.sent_count == 1U);
    bindings[0].adapter = nullptr;
    output::ConfiguredOutputEquipment unavailable(graph, bindings, graph.actuator_count);
    assert(!unavailable.valid() && unavailable.status() == output::EquipmentStatus::MissingAdapter);
    output::ConfiguredOutputTransport blocked(unavailable);
    assert(blocked.send(command).status == output::TransportStatus::NotAttempted);
    assert(!unavailable.capabilities().exhaust_fan && !unavailable.capabilities().heater);
    if (graph.actuator_count > 1U) {
      bindings[0].adapter = &first;
      bindings[1].endpoint = bindings[0].endpoint;
      output::ConfiguredOutputEquipment conflict(graph, bindings, graph.actuator_count);
      assert(conflict.status() == output::EquipmentStatus::DuplicateAdapter);
    }
  }
}

void testConfiguredDualDeviceHasIndependentPorts() {
  namespace ipo = growbox::ipo;
  ipo::Graph graph{};
  graph.processes[graph.process_count++] = {1U, ipo::ProcessKind::Co2Safety, true};
  graph.devices[graph.device_count++] = {42U, ipo::Driver::ShellySwitchMeter, 42U, true};
  graph.signals[graph.signal_count++] = {ipo::SignalRole::ActivePower, 42U, ipo::Channel::ActivePower, 0U};
  graph.signals[graph.signal_count++] = {ipo::SignalRole::Energy, 42U, ipo::Channel::Energy, 0U};
  graph.actuators[graph.actuator_count++] = {101U, 42U, ipo::Channel::Switch, ipo::OutputRole::Heater, true, 100U, 100U};
  assert(ipo::validateGraph(graph) == ipo::GraphStatus::Ok);
  DualPortFakeDevice device;
  output::ConfiguredOutputEquipment::Bindings bindings{};
  bindings[0] = {ipo::Driver::ShellySwitchMeter, 42U, 11U, 7U, &device};
  output::ConfiguredOutputEquipment equipment(graph, bindings, 1U);
  assert(equipment.valid() && equipment.capabilities().heater);
  output::ConfiguredOutputPolicies policies(equipment);
  output::OutputStateStore store;
  std::array<output::OutputEndpointId, output::kOutputEndpointCapacity> endpoints{11U};
  assert(store.configure(endpoints, 1U));
  output::ConfiguredOutputTransport transport(equipment);
  output::OutputSupervisorResolver resolver(policies.config());
  output::OutputSupervisorExecutor executor(transport, store, policies.config());
  climate_io::ClimateOutputSupervisorSink sink(equipment.semantic(), resolver, executor, store);
  climate_io::DeviceElectricalSnapshot sample{};
  climate_io::DeviceElectricalSource& input = device;
  assert(input.sampleElectrical(0U, sample));
  assert(sample.device_id == 42U && device.reads == 1U && device.writes == 0U);
  climate::ClimatePolicyRequest demand{};
  demand.heater = 1.0F;
  climate::ClimateExecutionProjection execution{};
  assert(sink.applyAndReportExecution(demand, 1000U, execution));
  assert(device.last_endpoint == 7U && device.writes == 1U && device.reads == 1U);
  assertPhysicalUnknown(store, 11U);
  assert(input.sampleElectrical(1010U, sample));
  assert(device.writes == 1U && device.reads == 2U);
  assertPhysicalUnknown(store, 11U);
  graph.devices[graph.device_count++] = {43U, ipo::Driver::ShellyMeter, 42U, true};
  assert(ipo::validateGraph(graph) == ipo::GraphStatus::DuplicatePhysicalEndpoint);
}

void testDualDeviceInputIsIndependentOfSupervisorOutput() {
  auto fan = makePolicy();
  auto humidifier = makePolicy();
  auto store = makeStore();
  const auto config = makeSupervisorConfig(fan, humidifier);
  output::OutputSupervisorResolver resolver(config);
  DualPortFakeDevice device;
  climate_io::DeviceElectricalSource& input = device;
  output::OutputTransport& transport = device;
  output::OutputSupervisorExecutor executor(transport, store, config);
  climate_io::ClimateOutputSupervisorSink sink(makeClimateConfig(), resolver, executor, store);
  sink.setCycleContext(scheduleContext(true));
  climate_io::DeviceElectricalSnapshot electrical{};
  assert(input.sampleElectrical(1'000U, electrical));
  assert(electrical.device_id == 42U);
  assert(electrical.active_power_w.value == 12.0F);
  assert(electrical.energy_wh.value == 150.0F);
  assert(device.reads == 1U && device.writes == 0U);
  climate::ClimateExecutionProjection execution{};
  assert(sink.applyAndReportExecution(request(1.0F, 1.0F), 1'000U, execution));
  assert(device.writes == 3U && device.reads == 1U);
  assertPhysicalUnknown(store, kFan);
  assertPhysicalUnknown(store, kLamp);
  assertPhysicalUnknown(store, kHumidifier);
}

void testCompleteClimateRequestUsesOneSupervisorCycleAndProjectsCommandTruth() {
  auto fan = makePolicy();
  auto humidifier = makePolicy();
  auto store = makeStore();
  const auto supervisor_config = makeSupervisorConfig(fan, humidifier);
  output::OutputSupervisorResolver resolver(supervisor_config);
  FakeTransport transport;
  output::OutputSupervisorExecutor executor(transport, store, supervisor_config);
  climate_io::ClimateOutputSupervisorSink sink(makeClimateConfig(), resolver, executor, store);
  assert(sink.valid());
  sink.setCycleContext(scheduleContext(true));

  climate::ClimatePolicyRequest projection{};
  assert(sink.applyAndReport(request(1.0F, 1.0F), 1'000U, projection));
  assert(transport.sent_count == 3U);
  assert(transport.sent[0].endpoint == kFan);
  assert(transport.sent[1].endpoint == kLamp);
  assert(transport.sent[2].endpoint == kHumidifier);
  assert(sink.lastReport().size == 3U);
  assert(near(projection.exhaust_fan, 1.0F));
  assert(near(projection.humidifier, 1.0F));
  assert(near(projection.heater, 0.0F));
  assert(near(projection.cooler, 0.0F));
  assert(near(projection.dehumidifier, 0.0F));
  assert(near(projection.co2_doser, 0.0F));
  assertPhysicalUnknown(store, kFan);
  assertPhysicalUnknown(store, kLamp);
  assertPhysicalUnknown(store, kHumidifier);

  assert(store.recordPhysicalObservation(kFan, output::PhysicalOutputState::Off, 1'010U, 1U));
  assert(
      store.recordPhysicalObservation(kHumidifier, output::PhysicalOutputState::Off, 1'010U, 2U));
  sink.setCycleContext(scheduleContext(true));
  assert(sink.applyAndReport(request(1.0F, 1.0F), 2'000U, projection));
  assert(transport.sent_count == 3U);
  assert(sink.lastReport().size == 0U);
  assert(near(projection.exhaust_fan, 1.0F));
  assert(near(projection.humidifier, 1.0F));
  assert(store.find(kFan)->physical.state == output::PhysicalOutputState::Off);
  assert(store.find(kHumidifier)->physical.state == output::PhysicalOutputState::Off);
}

void testExecutionFeedbackIsCompleteCommandTruthAndNotPhysicalAck() {
  auto fan = makePolicy();
  auto humidifier = makePolicy();
  auto store = makeStore();
  const auto supervisor_config = makeSupervisorConfig(fan, humidifier);
  output::OutputSupervisorResolver resolver(supervisor_config);
  FakeTransport transport;
  output::OutputSupervisorExecutor executor(transport, store, supervisor_config);
  climate_io::ClimateOutputSupervisorSink sink(makeClimateConfig(), resolver, executor, store);
  sink.setCycleContext(scheduleContext(true));

  climate::ClimateExecutionProjection execution{};
  assert(sink.applyAndReportExecution(request(1.0F, 1.0F), 5'000U, execution));
  assert(execution.known_mask == climate::ClimateExecutionKnownAll);
  assert(execution.known(climate::ClimateExecutionKnownHeater));
  assert(execution.known(climate::ClimateExecutionKnownCooler));
  assert(execution.known(climate::ClimateExecutionKnownExhaustFan));
  assert(execution.known(climate::ClimateExecutionKnownHumidifier));
  assert(execution.known(climate::ClimateExecutionKnownDehumidifier));
  assert(execution.known(climate::ClimateExecutionKnownCo2Doser));
  assert(near(execution.executed.heater, 0.0F));
  assert(near(execution.executed.cooler, 0.0F));
  assert(near(execution.executed.exhaust_fan, 1.0F));
  assert(near(execution.executed.humidifier, 1.0F));
  assert(near(execution.executed.dehumidifier, 0.0F));
  assert(near(execution.executed.co2_doser, 0.0F));
  assertPhysicalUnknown(store, kFan);
  assertPhysicalUnknown(store, kHumidifier);
}

void testArmingReportsPartialCommandTruthWithoutInventingUnknownState() {
  auto fan = makePolicy();
  auto humidifier = makePolicy();
  auto store = makeStore();
  assert(store.restoreLastSuccessfulCommand(kFan, output::BinaryOutputState::Off));
  const auto supervisor_config = makeSupervisorConfig(fan, humidifier);
  output::OutputSupervisorResolver resolver(supervisor_config);
  FakeTransport transport;
  output::OutputSupervisorExecutor executor(transport, store, supervisor_config);
  climate_io::ClimateOutputSupervisorSink sink(makeClimateConfig(), resolver, executor, store);

  auto context = scheduleContext(false);
  context.mode = output::SupervisorMode::Arming;
  sink.setCycleContext(context);

  climate::ClimateExecutionProjection execution{};
  assert(sink.applyAndReportExecution(request(1.0F, 1.0F), 6'000U, execution));
  assert(transport.sent_count == 0U);
  assert(sink.lastReport().size == 0U);
  assert(execution.known(climate::ClimateExecutionKnownHeater));
  assert(execution.known(climate::ClimateExecutionKnownCooler));
  assert(execution.known(climate::ClimateExecutionKnownExhaustFan));
  assert(!execution.known(climate::ClimateExecutionKnownHumidifier));
  assert(execution.known(climate::ClimateExecutionKnownDehumidifier));
  assert(execution.known(climate::ClimateExecutionKnownCo2Doser));
  assert(near(execution.executed.exhaust_fan, 0.0F));
  assert(near(execution.executed.humidifier, 0.0F));

  climate::ClimatePolicyRequest compatibility_projection{};
  sink.setCycleContext(context);
  assert(!sink.applyAndReport(request(1.0F, 1.0F), 6'001U, compatibility_projection));
  assert(transport.sent_count == 0U);
}

void testPartialFailureReturnsFalseAndProjectsLastSuccessfulCommands() {
  auto fan = makePolicy();
  auto humidifier = makePolicy();
  auto store = makeStore();
  const auto supervisor_config = makeSupervisorConfig(fan, humidifier);
  output::OutputSupervisorResolver resolver(supervisor_config);
  FakeTransport transport;
  output::OutputSupervisorExecutor executor(transport, store, supervisor_config);
  climate_io::ClimateOutputSupervisorSink sink(makeClimateConfig(), resolver, executor, store);
  sink.setCycleContext(scheduleContext(false));

  climate::ClimatePolicyRequest projection{};
  assert(sink.applyAndReport(request(0.0F, 0.0F), 10'000U, projection));
  assert(transport.sent_count == 3U);
  assert(near(projection.exhaust_fan, 0.0F));
  assert(near(projection.humidifier, 0.0F));

  transport.scripted_count = 2U;
  transport.scripted_index = 0U;
  transport.scripted[0] = {output::TransportStatus::Failed, output::TransportError::IoFailure};
  transport.scripted[1] = {output::TransportStatus::Completed, output::TransportError::None};
  sink.setCycleContext(scheduleContext(false));
  assert(!sink.applyAndReport(request(1.0F, 1.0F), 20'000U, projection));
  assert(sink.lastReport().size == 2U);
  assert(sink.lastReport().steps[0].command.endpoint == kFan);
  assert(sink.lastReport().steps[0].transport.status == output::TransportStatus::Failed);
  assert(sink.lastReport().steps[1].command.endpoint == kHumidifier);
  assert(sink.lastReport().steps[1].transport.status == output::TransportStatus::Completed);
  assert(near(projection.exhaust_fan, 0.0F));
  assert(near(projection.humidifier, 1.0F));
  assert(store.find(kFan)->last_successful_command.state == output::BinaryOutputState::Off);
  assert(store.find(kHumidifier)->last_successful_command.state == output::BinaryOutputState::On);
  assertPhysicalUnknown(store, kFan);
  assertPhysicalUnknown(store, kHumidifier);
}

void testDwellHoldReturnsHeldExecutionProjectionWithoutTransport() {
  auto fan = makePolicy(120'000U, 120'000U);
  auto humidifier = makePolicy();
  auto store = makeStore();
  const auto supervisor_config = makeSupervisorConfig(fan, humidifier);
  output::OutputSupervisorResolver resolver(supervisor_config);
  FakeTransport transport;
  output::OutputSupervisorExecutor executor(transport, store, supervisor_config);
  climate_io::ClimateOutputSupervisorSink sink(makeClimateConfig(), resolver, executor, store);
  sink.setCycleContext(scheduleContext(false));

  climate::ClimatePolicyRequest projection{};
  assert(sink.applyAndReport(request(0.0F, 0.0F), 100'000U, projection));
  const std::size_t after_initial = transport.sent_count;
  assert(after_initial == 3U);

  sink.setCycleContext(scheduleContext(false));
  assert(sink.applyAndReport(request(1.0F, 0.0F), 100'001U, projection));
  assert(transport.sent_count == after_initial);
  assert(sink.lastReport().size == 0U);
  assert(near(projection.exhaust_fan, 0.0F));
  assert(fan.dwellHoldCount() == 1U);
}

void testSafetyContextOverridesClimateWithoutChangingProjectionMeaning() {
  auto fan = makePolicy();
  auto humidifier = makePolicy();
  auto store = makeStore();
  const auto supervisor_config = makeSupervisorConfig(fan, humidifier);
  output::OutputSupervisorResolver resolver(supervisor_config);
  FakeTransport transport;
  output::OutputSupervisorExecutor executor(transport, store, supervisor_config);
  climate_io::ClimateOutputSupervisorSink sink(makeClimateConfig(), resolver, executor, store);

  auto context = scheduleContext(true);
  context.safety.metadata.sequence = 90U;
  context.safety.metadata.source = output::OutputSource::Safety;
  context.safety.metadata.reason = output::OutputReason::ThermalSafety;
  assert(output::setSafetyConstraint(context.safety.endpoints[0], kFan,
                                     output::SafetyConstraint::ForceOn,
                                     output::OutputReason::ThermalSafety));
  assert(output::setSafetyConstraint(context.safety.endpoints[1], kLamp,
                                     output::SafetyConstraint::ForceOff,
                                     output::OutputReason::ThermalSafety));
  sink.setCycleContext(context);

  climate::ClimatePolicyRequest projection{};
  assert(sink.applyAndReport(request(0.0F, 0.0F), 30'000U, projection));
  assert(transport.sent_count == 3U);
  assert(transport.sent[0].endpoint == kFan);
  assert(transport.sent[0].state == output::BinaryOutputState::On);
  assert(transport.sent[1].endpoint == kLamp);
  assert(transport.sent[1].state == output::BinaryOutputState::Off);
  assert(near(projection.exhaust_fan, 1.0F));
  assert(near(projection.humidifier, 0.0F));
}

void testRejectsUnsupportedOrNonFiniteClimateRequestBeforeTransport() {
  auto fan = makePolicy();
  auto humidifier = makePolicy();
  auto store = makeStore();
  const auto supervisor_config = makeSupervisorConfig(fan, humidifier);
  output::OutputSupervisorResolver resolver(supervisor_config);
  FakeTransport transport;
  output::OutputSupervisorExecutor executor(transport, store, supervisor_config);
  climate_io::ClimateOutputSupervisorSink sink(makeClimateConfig(), resolver, executor, store);

  climate::ClimatePolicyRequest projection{};
  auto unsupported = request(0.0F, 0.0F);
  unsupported.heater = 0.2F;
  assert(!sink.applyAndReport(unsupported, 40'000U, projection));
  assert(transport.sent_count == 0U);

  auto invalid = request(0.0F, 0.0F);
  invalid.exhaust_fan = std::numeric_limits<float>::quiet_NaN();
  assert(!sink.applyAndReport(invalid, 40'001U, projection));
  assert(transport.sent_count == 0U);
}

void testFailSafeOffExecutesThroughSupervisorOnly() {
  auto fan = makePolicy();
  auto humidifier = makePolicy();
  auto store = makeStore();
  const auto supervisor_config = makeSupervisorConfig(fan, humidifier);
  output::OutputSupervisorResolver resolver(supervisor_config);
  FakeTransport transport;
  output::OutputSupervisorExecutor executor(transport, store, supervisor_config);
  climate_io::ClimateOutputSupervisorSink sink(makeClimateConfig(), resolver, executor, store);
  sink.setCycleContext(scheduleContext(true));

  climate::ClimatePolicyRequest projection{};
  assert(sink.applyAndReport(request(1.0F, 1.0F), 50'000U, projection));
  assert(transport.sent_count == 3U);
  assert(store.find(kLamp)->last_successful_command.state == output::BinaryOutputState::On);

  sink.setCycleContext(scheduleContext(true));
  assert(sink.applyFailSafeOff(50'001U));
  assert(transport.sent_count == 5U);
  assert(sink.lastReport().size == 2U);
  assert(transport.sent[3].endpoint == kFan);
  assert(transport.sent[3].state == output::BinaryOutputState::Off);
  assert(transport.sent[3].source == output::OutputSource::Safety);
  assert(transport.sent[3].reason == output::OutputReason::FaultContainment);
  assert(transport.sent[4].endpoint == kHumidifier);
  assert(transport.sent[4].state == output::BinaryOutputState::Off);
  assert(transport.sent[4].source == output::OutputSource::Safety);
  assert(transport.sent[4].reason == output::OutputReason::FaultContainment);
  assert(store.find(kLamp)->last_successful_command.state == output::BinaryOutputState::On);
  assertPhysicalUnknown(store, kFan);
  assertPhysicalUnknown(store, kHumidifier);
}

void testHardSafetyOverridesSupervisorFailSafeOff() {
  auto fan = makePolicy();
  auto humidifier = makePolicy();
  auto store = makeStore();
  const auto supervisor_config = makeSupervisorConfig(fan, humidifier);
  output::OutputSupervisorResolver resolver(supervisor_config);
  FakeTransport transport;
  output::OutputSupervisorExecutor executor(transport, store, supervisor_config);
  climate_io::ClimateOutputSupervisorSink sink(makeClimateConfig(), resolver, executor, store);
  sink.setCycleContext(scheduleContext(false));

  climate::ClimatePolicyRequest projection{};
  assert(sink.applyAndReport(request(0.0F, 0.0F), 60'000U, projection));
  assert(transport.sent_count == 3U);

  auto context = scheduleContext(false);
  context.safety.metadata.sequence = 91U;
  context.safety.metadata.source = output::OutputSource::Safety;
  context.safety.metadata.reason = output::OutputReason::ThermalSafety;
  assert(output::setSafetyConstraint(context.safety.endpoints[0], kFan,
                                     output::SafetyConstraint::ForceOn,
                                     output::OutputReason::ThermalSafety));
  sink.setCycleContext(context);

  assert(sink.applyFailSafeOff(60'001U));
  assert(transport.sent_count == 4U);
  assert(sink.lastReport().size == 1U);
  assert(transport.sent[3].endpoint == kFan);
  assert(transport.sent[3].state == output::BinaryOutputState::On);
  assert(transport.sent[3].source == output::OutputSource::Safety);
  assert(transport.sent[3].reason == output::OutputReason::ThermalSafety);
  assert(store.find(kHumidifier)->last_successful_command.state == output::BinaryOutputState::Off);
}

} // namespace

int main() {
  testMintEquipmentParity();
  testConfiguredSupervisorAllRoles();
  testConfiguredEquipmentAndTransportSubstitution();
  testConfiguredDualDeviceHasIndependentPorts();
  testDualDeviceInputIsIndependentOfSupervisorOutput();
  testCompleteClimateRequestUsesOneSupervisorCycleAndProjectsCommandTruth();
  testExecutionFeedbackIsCompleteCommandTruthAndNotPhysicalAck();
  testArmingReportsPartialCommandTruthWithoutInventingUnknownState();
  testPartialFailureReturnsFalseAndProjectsLastSuccessfulCommands();
  testDwellHoldReturnsHeldExecutionProjectionWithoutTransport();
  testSafetyContextOverridesClimateWithoutChangingProjectionMeaning();
  testRejectsUnsupportedOrNonFiniteClimateRequestBeforeTransport();
  testFailSafeOffExecutesThroughSupervisorOnly();
  testHardSafetyOverridesSupervisorFailSafeOff();
  return 0;
}
