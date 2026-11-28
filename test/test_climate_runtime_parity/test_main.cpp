#include "ClimatePolicy.h"
#include "ClimateRuntimeController.h"
#include "climate_runtime_parity_generated.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>

namespace {
int failures = 0;

void check(bool condition, const char* case_name, const char* message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL [" << case_name << "]: " << message << '\n';
  }
}

bool near(float left, float right, float tolerance = 1.0e-4F) {
  return std::abs(left - right) <= tolerance;
}

std::array<float, 6> values(const growbox::climate::ClimatePolicyRequest& request) {
  return {request.heater,     request.cooler,       request.exhaust_fan,
          request.humidifier, request.dehumidifier, request.co2_doser};
}

std::array<float, 6> values(const growbox::climate::EstimatedEffectiveClimateActions& action) {
  return {action.heater,     action.cooler,       action.exhaust_fan,
          action.humidifier, action.dehumidifier, action.co2_doser};
}

template <std::size_t N>
void checkArray(const std::array<float, N>& actual, const std::array<float, N>& expected,
                const char* case_name, const char* message, float tolerance = 1.0e-4F) {
  for (std::size_t index = 0; index < N; ++index) {
    if (!near(actual[index], expected[index], tolerance)) {
      ++failures;
      std::cerr << "FAIL [" << case_name << "]: " << message << " index=" << index
                << " actual=" << actual[index] << " expected=" << expected[index] << '\n';
      return;
    }
  }
}

class FixtureProvider final : public growbox::climate::ClimateInferenceProvider {
public:
  explicit FixtureProvider(const growbox::climate::parity_fixture::Case& fixture)
      : fixture_(fixture) {}

  bool infer(const growbox::climate::ClimateFeatureVector&,
             growbox::climate::ClimatePolicyRequest& output) noexcept override {
    if (fixture_.model_behavior == growbox::climate::parity_fixture::ModelBehavior::Fail) {
      return false;
    }
    const auto& value = fixture_.model_output;
    output = {value[0], value[1], value[2], value[3], value[4], value[5]};
    return true;
  }

private:
  const growbox::climate::parity_fixture::Case& fixture_;
};

growbox::climate::ClimateControllerInput
makeInput(const growbox::climate::parity_fixture::Case& fixture) {
  using namespace growbox::climate;
  ClimateControllerInput input{};
  auto measured = [&](std::size_t index) {
    return MeasuredValue{fixture.measurement_values[index], fixture.measurement_valid[index],
                         fixture.measurement_age_ms[index]};
  };
  input.state.measurements.air_temperature_c = measured(0U);
  input.state.measurements.relative_humidity_pct = measured(1U);
  input.state.measurements.co2_ppm = measured(2U);
  input.state.measurements.outside_temperature_c = measured(3U);
  input.state.measurements.outside_humidity_pct = measured(4U);
  input.humidity_control_mode =
      fixture.humidity_vpd ? HumidityControlMode::Vpd : HumidityControlMode::Rh;
  input.targets.air_temperature_c = fixture.target_temperature_c;
  input.targets.relative_humidity_pct = fixture.target_humidity_pct;
  input.targets.air_vpd_kpa = fixture.target_vpd_kpa;
  input.targets.co2_enabled = fixture.co2_enabled;
  input.targets.co2_ppm = fixture.target_co2_ppm;
  input.schedule.light_level = fixture.light_level;
  input.capabilities = {fixture.capabilities[0], fixture.capabilities[1], fixture.capabilities[2],
                        fixture.capabilities[3], fixture.capabilities[4], fixture.capabilities[5]};
  input.previous = {fixture.previous[0], fixture.previous[1], fixture.previous[2],
                    fixture.previous[3], fixture.previous[4], fixture.previous[5]};
  input.sensor_timeout_ms = fixture.sensor_timeout_ms;
  return input;
}

void runCase(const growbox::climate::parity_fixture::Case& fixture) {
  using namespace growbox::climate;
  ClimateRuntimeConfig config{};
  config.mode = static_cast<ClimatePolicyMode>(fixture.mode);
  config.allow_unqualified_ml_active = fixture.allow_ml_active;
  config.sensor_timeout_ms = fixture.sensor_timeout_ms;
  config.timestep_s = fixture.timestep_s;
  config.ml_deadzone = 0.05F;

  FixtureProvider provider{fixture};
  ClimateInferenceProvider* provider_ptr =
      fixture.model_behavior == parity_fixture::ModelBehavior::None ? nullptr : &provider;
  ClimateRuntimeController runtime{provider_ptr, config};
  ClimateRuntimeDecision decision{};
  const auto status = runtime.step(makeInput(fixture), fixture.monotonic_ms, decision);

  check(static_cast<std::uint8_t>(status) == fixture.expected_status, fixture.name,
        "runtime status");
  check(decision.authoritative_ml == fixture.expected_authoritative_ml, fixture.name,
        "authoritative policy");
  check(decision.ml_evaluated == fixture.expected_ml_evaluated, fixture.name, "ML evaluated");

  checkArray(values(decision.rule.raw), fixture.expected_rule_raw, fixture.name, "rule raw");
  checkArray(values(decision.rule.arbitrated), fixture.expected_rule_arbitrated, fixture.name,
             "rule arbitrated");
  checkArray(values(decision.rule.safe), fixture.expected_rule_safe, fixture.name, "rule safe");
  check(decision.rule.arbitration_interventions == fixture.expected_rule_arb_mask, fixture.name,
        "rule arbitration interventions");
  check(decision.rule.safety_interventions == fixture.expected_rule_safety_mask, fixture.name,
        "rule safety interventions");

  if (fixture.expected_ml_evaluated) {
    checkArray(values(decision.ml.raw), fixture.expected_ml_raw, fixture.name, "ML raw");
    checkArray(values(decision.ml.arbitrated), fixture.expected_ml_arbitrated, fixture.name,
               "ML arbitrated");
    checkArray(values(decision.ml.safe), fixture.expected_ml_safe, fixture.name, "ML safe");
    check(decision.ml.arbitration_interventions == fixture.expected_ml_arb_mask, fixture.name,
          "ML arbitration interventions");
    check(decision.ml.safety_interventions == fixture.expected_ml_safety_mask, fixture.name,
          "ML safety interventions");
  }

  if (fixture.expected_has_ml_features) {
    checkArray(decision.ml_features.values, fixture.expected_ml_features, fixture.name,
               "44-feature runtime vector", 2.0e-4F);
  }

  checkArray(values(decision.applied), fixture.expected_applied, fixture.name, "applied action");
  checkArray(values(decision.effective_before), fixture.expected_effective_before, fixture.name,
             "effective before");
  checkArray(values(decision.effective_after), fixture.expected_effective_after, fixture.name,
             "effective after", 2.0e-4F);
}
void testCo2TargetVentilation() {
  using namespace growbox::climate;
  ClimateControllerInput input{};
  auto& m = input.state.measurements;
  m.air_temperature_c = {24.0F, true, 0U};
  m.relative_humidity_pct = {60.0F, true, 0U};
  input.targets.air_temperature_c = 24.0F;
  input.targets.relative_humidity_pct = 60.0F;
  input.targets.co2_enabled = true;
  input.targets.co2_ppm = 900.0F;
  input.capabilities.exhaust_fan = true;
  input.capabilities.co2_doser = true;
  struct Scenario {
    const char* name;
    float ppm;
    bool fresh;
    bool enabled;
    bool fan;
    bool doser;
    float expected_fan;
    float expected_doser;
  };
  const Scenario scenarios[] = {
      {"CO2 excess below safety", 1200.0F, true, true, true, true, 0.5F, 0.0F},
      {"CO2 high without doser", 1200.0F, true, true, true, false, 0.5F, 0.0F},
      {"CO2 no fan", 1200.0F, true, true, false, true, 0.0F, 0.0F},
      {"CO2 deadband", 950.0F, true, true, true, true, 0.0F, 0.0F},
      {"CO2 deficit", 600.0F, true, true, true, true, 0.0F, 0.5F},
      {"CO2 stale", 1200.0F, false, true, true, true, 0.0F, 0.0F},
      {"CO2 disabled", 1200.0F, true, false, true, true, 0.0F, 0.0F},
      {"CO2 safety independent", 1800.0F, true, false, true, true, 1.0F, 0.0F},
  };
  for (const auto& s : scenarios) {
    m.co2_ppm = {s.ppm, true, s.fresh ? 0U : input.sensor_timeout_ms + 1U};
    input.targets.co2_enabled = s.enabled;
    input.capabilities.exhaust_fan = s.fan;
    input.capabilities.co2_doser = s.doser;
    ClimateRuntimeController runtime{nullptr, {}};
    ClimateRuntimeDecision decision{};
    runtime.step(input, 1000U, decision);
    check(near(decision.rule.safe.exhaust_fan, s.expected_fan), s.name, "ventilation");
    check(near(decision.rule.safe.co2_doser, s.expected_doser), s.name, "dosing");
  }
}
void testDirectionalVentilation() {
  using namespace growbox::climate;
  ClimateControllerInput in{};
  in.capabilities.exhaust_fan = true;
  in.targets.air_temperature_c = 24.0F;
  in.targets.relative_humidity_pct = 60.0F;
  auto& m = in.state.measurements;
  m.air_temperature_c = {28.0F, true, 0U};
  m.relative_humidity_pct = {60.0F, true, 0U};
  m.outside_temperature_c = {15.0F, true, 0U};
  m.outside_humidity_pct = {60.0F, true, 0U};
  auto fan = [&] { return policy::ruleRequest(in).exhaust_fan; };
  check(near(fan(), 0.925F), "directional_ventilation", "cooling gradient capped by target error");
  m.outside_temperature_c.value = 35.0F;
  check(near(fan(), 0.0F), "directional_ventilation", "hot intake cannot cool");
  m.outside_temperature_c = {15.0F, false, 0U};
  check(near(fan(), 0.0F), "directional_ventilation", "unknown intake cannot cool");
  m.outside_temperature_c = {15.0F, true, 0U};
  m.outside_humidity_pct.valid = false;
  check(near(fan(), 0.925F), "directional_ventilation", "temperature opportunity does not require RH");
  m.air_temperature_c.value = 24.0F;
  m.relative_humidity_pct.value = 80.0F;
  m.outside_humidity_pct = {85.0F, true, 0U};
  check(fan() > 0.0F, "directional_ventilation", "cold high-RH intake can carry less moisture");
  m.outside_temperature_c.value = 35.0F;
  m.outside_humidity_pct.value = 65.0F;
  check(near(fan(), 0.0F), "directional_ventilation", "warm low-RH intake can carry more moisture");
  m.relative_humidity_pct.value = 35.0F;
  check(fan() > 0.0F, "directional_ventilation", "moist intake opportunity");
  m.air_temperature_c.value = 28.0F;
  check(near(fan(), 0.0F), "directional_ventilation", "moisture ventilation must not worsen temperature");
  m.air_temperature_c.value = 24.0F;
  in.humidity_control_mode = HumidityControlMode::Vpd;
  in.targets.air_vpd_kpa = 1.2F;
  check(fan() > 0.0F, "directional_ventilation", "VPD moisture opportunity");
  in.capabilities.exhaust_fan = false;
  check(near(fan(), 0.0F), "directional_ventilation", "unavailable fan");
}
void testLampTrendAndConflicts() {
  using namespace growbox::climate;
  ClimateControllerInput in{};
  in.targets.air_temperature_c = 24.0F;
  in.targets.relative_humidity_pct = 60.0F;
  in.targets.co2_enabled = true;
  in.targets.co2_ppm = 900.0F;
  in.capabilities.exhaust_fan = true;
  in.capabilities.co2_doser = true;
  auto& m = in.state.measurements;
  m.air_temperature_c = {24.0F, true, 0U};
  m.relative_humidity_pct = {60.0F, true, 0U};
  m.co2_ppm = {600.0F, true, 0U};
  m.outside_temperature_c = {20.0F, true, 0U};
  m.outside_humidity_pct = {60.0F, true, 0U};
  in.schedule.light_level = 1.0F;
  auto request = [&] { return policy::ruleRequest(in); };
  check(near(request().exhaust_fan, 0.0F), "lamp_conflict", "schedule alone cannot imply heat");
  in.state.trends.temperature = {0.6F, true};
  check(near(request().exhaust_fan, 0.5F), "lamp_conflict", "observed rise and cool intake");
  check(near(request().co2_doser, 0.0F), "lamp_conflict", "ventilation wins over enrichment");
  in.schedule.light_level = 0.0F;
  check(near(request().exhaust_fan, 0.0F), "lamp_conflict", "no lamp anticipation at night");
  check(near(request().co2_doser, 0.5F), "lamp_conflict", "enrichment resumes when not ventilating");
  in.schedule.light_level = 1.0F;
  m.outside_temperature_c.value = 30.0F;
  check(near(request().exhaust_fan, 0.0F), "lamp_conflict", "hot intake cannot anticipate cooling");
  in.capabilities.cooler = true;
  check(near(request().cooler, 0.5F), "lamp_conflict", "direct cooler can anticipate without exchange");
  check(near(request().co2_doser, 0.5F), "lamp_conflict", "direct cooling can coexist with dosing");
  in.state.trends.temperature = {-0.6F, true};
  check(near(request().cooler, 0.0F), "lamp_conflict", "falling trend does not anticipate");
  in.state.trends.temperature = {0.6F, false};
  check(near(request().cooler, 0.0F), "lamp_conflict", "unknown trend does not anticipate");
  m.air_temperature_c.value = 28.0F;
  m.outside_temperature_c.value = 20.0F;
  check(request().exhaust_fan > 0.0F && near(request().co2_doser, 0.0F),
        "lamp_conflict", "thermal ventilation suppresses enrichment regardless of lamp");
  m.air_temperature_c.value = 24.0F;
  m.relative_humidity_pct.value = 80.0F;
  check(request().exhaust_fan > 0.0F && near(request().co2_doser, 0.0F),
        "lamp_conflict", "moisture ventilation suppresses enrichment");
  m.relative_humidity_pct.value = 60.0F;
  for (unsigned i = 0; i < 32; ++i) {
    m.air_temperature_c.value = 24.0F + ((i % 2U) ? 0.1F : -0.1F);
    check(near(request().exhaust_fan, 0.0F) && near(request().cooler, 0.0F),
          "lamp_conflict", "deadband repeatability with no valid trend");
  }
}

} // namespace

int main() {
  testLampTrendAndConflicts();
  testDirectionalVentilation();
  testCo2TargetVentilation();
  for (const auto& fixture : growbox::climate::parity_fixture::kCases) {
    runCase(fixture);
  }
  if (failures != 0) {
    std::cerr << failures << " climate runtime parity checks failed\n";
    return 1;
  }
  std::cout << "climate runtime parity checks passed\n";
  return 0;
}
