#include "climate/runtime/profile/IpoProfileJsonDecode.h"

#include <cJSON.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace growbox::app::climate_io::runtime::profile {
namespace {

using namespace ::growbox::ipo;

template <std::size_t N>
bool exactObject(const cJSON* object, const char* const (&fields)[N]) noexcept {
  if (!cJSON_IsObject(object)) return false;
  std::uint32_t seen = 0U;
  const cJSON* child = nullptr;
  cJSON_ArrayForEach(child, object) {
    if (child->string == nullptr) return false;
    bool found = false;
    for (std::size_t i = 0; i < N; ++i) {
      if (std::strcmp(child->string, fields[i]) == 0) {
        const auto bit = static_cast<std::uint32_t>(1U << i);
        if ((seen & bit) != 0U) return false; // duplicate keys fail closed
        seen |= bit;
        found = true;
        break;
      }
    }
    if (!found) return false;
  }
  return seen == ((static_cast<std::uint32_t>(1U) << N) - 1U);
}

const cJSON* item(const cJSON* parent, const char* key) noexcept {
  return cJSON_IsObject(parent) ? cJSON_GetObjectItemCaseSensitive(parent, key) : nullptr;
}
const char* stringValue(const cJSON* value) noexcept {
  return cJSON_IsString(value) && value->valuestring != nullptr ? value->valuestring : nullptr;
}
const char* stringItem(const cJSON* parent, const char* key) noexcept {
  return stringValue(item(parent, key));
}
bool equals(const char* left, const char* right) noexcept {
  return left != nullptr && right != nullptr && std::strcmp(left, right) == 0;
}
bool validIdentifier(const char* value) noexcept {
  if (value == nullptr) return false;
  const auto size = std::strlen(value);
  if (size < 2U || size > 40U || value[0] < 'a' || value[0] > 'z') return false;
  for (std::size_t i = 1U; i < size; ++i) {
    const char c = value[i];
    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_'))
      return false;
  }
  return true;
}
bool nonEmptyLabel(const char* value) noexcept {
  return value != nullptr && value[0] != '\0' && std::strlen(value) <= 80U;
}
bool booleanItem(const cJSON* parent, const char* key, bool& output) noexcept {
  const cJSON* node = item(parent, key);
  if (!cJSON_IsBool(node)) return false;
  output = cJSON_IsTrue(node);
  return true;
}
bool integerItem(const cJSON* parent, const char* key, std::uint32_t min,
                 std::uint32_t max, std::uint32_t& output) noexcept {
  const cJSON* node = item(parent, key);
  if (!cJSON_IsNumber(node) || !std::isfinite(node->valuedouble) ||
      node->valuedouble < static_cast<double>(min) ||
      node->valuedouble > static_cast<double>(max) ||
      std::floor(node->valuedouble) != node->valuedouble) return false;
  output = static_cast<std::uint32_t>(node->valuedouble);
  return true;
}
bool floatItem(const cJSON* parent, const char* key, float& output) noexcept {
  const cJSON* node = item(parent, key);
  if (!cJSON_IsNumber(node) || !std::isfinite(node->valuedouble) ||
      std::fabs(node->valuedouble) > std::numeric_limits<float>::max())
    return false;
  output = static_cast<float>(node->valuedouble);
  return std::isfinite(output);
}
bool arraySize(const cJSON* parent, const char* key, int maximum, int& size,
               bool required_nonempty = false) noexcept {
  const cJSON* value = item(parent, key);
  if (!cJSON_IsArray(value)) return false;
  size = cJSON_GetArraySize(value);
  return size >= (required_nonempty ? 1 : 0) && size <= maximum;
}

template <typename T>
struct NamedEnum { const char* name; T value; };
template <typename T, std::size_t N>
bool enumValue(const char* source, const NamedEnum<T> (&catalog)[N], T& output) noexcept {
  if (source == nullptr) return false;
  for (const auto& option : catalog) {
    if (equals(source, option.name)) {
      output = option.value;
      return true;
    }
  }
  return false;
}

constexpr NamedEnum<Driver> kDrivers[]{
  {"ble.tp357", Driver::Tp357}, {"ble.xiaomi", Driver::Xiaomi},
  {"i2c.scd41", Driver::Scd41}, {"i2c.ds3231", Driver::Ds3231},
  {"i2c.bme280", Driver::Bme280}, {"i2c.bh1750", Driver::Bh1750},
  {"adc.soil", Driver::SoilAdc}, {"rf433.binary_socket", Driver::Rf433BinarySocket},
  {"gpio.relay", Driver::GpioRelay}, {"pwm.channel", Driver::PwmChannel},
  {"modbus.relay", Driver::ModbusRelay},
};
constexpr NamedEnum<Channel> kChannels[]{
  {"temperature_c", Channel::Temperature}, {"humidity_pct", Channel::Humidity},
  {"co2_ppm", Channel::Co2}, {"unix_time", Channel::UnixTime},
  {"illuminance_lux", Channel::Illuminance}, {"moisture_pct", Channel::SoilMoisture},
  {"switch", Channel::Switch}, {"duty", Channel::Duty},
};
constexpr NamedEnum<SignalRole> kSignals[]{
  {"inside.temperature_c", SignalRole::InsideTemperature},
  {"inside.humidity_pct", SignalRole::InsideHumidity},
  {"inside.co2_ppm", SignalRole::InsideCo2},
  {"outside.temperature_c", SignalRole::OutsideTemperature},
  {"outside.humidity_pct", SignalRole::OutsideHumidity},
  {"clock.time", SignalRole::ClockTime},
  {"light.illuminance_lux", SignalRole::LightIlluminance},
  {"substrate.moisture_pct", SignalRole::SubstrateMoisture},
};
constexpr NamedEnum<OutputRole> kOutputs[]{
  {"exhaust_fan", OutputRole::Exhaust}, {"humidifier", OutputRole::Humidifier},
  {"light", OutputRole::Light}, {"heater", OutputRole::Heater},
  {"cooler", OutputRole::Cooler}, {"dehumidifier", OutputRole::Dehumidifier},
  {"co2_doser", OutputRole::Co2Doser}, {"circulation_fan", OutputRole::CirculationFan},
  {"irrigation_pump", OutputRole::IrrigationPump},
};
constexpr NamedEnum<ProcessKind> kProcesses[]{
  {"temperature", ProcessKind::Temperature}, {"humidity", ProcessKind::Humidity},
  {"co2_safety", ProcessKind::Co2Safety}, {"co2_target", ProcessKind::Co2Target},
  {"light_schedule", ProcessKind::LightSchedule},
  {"circulation", ProcessKind::Circulation}, {"irrigation", ProcessKind::Irrigation},
};
constexpr NamedEnum<Effect> kEffects[]{
  {"ventilate", Effect::Ventilate}, {"humidify", Effect::Humidify},
  {"illuminate", Effect::Illuminate}, {"heat", Effect::Heat}, {"cool", Effect::Cool},
  {"dehumidify", Effect::Dehumidify}, {"dose_co2", Effect::DoseCo2},
  {"circulate", Effect::Circulate}, {"irrigate", Effect::Irrigate},
};

bool physicalLocator(Driver driver, const char* source, std::uint32_t& target) noexcept {
  // Only current board identities are understood. Other legal v2 driver/locator
  // strings belong to a future driver-specific adapter, never guessed here.
  if (driver == Driver::Tp357 && equals(source, "runtime:tp357")) target = 1U;
  else if (driver == Driver::Xiaomi && equals(source, "runtime:xiaomi")) target = 2U;
  else if (driver == Driver::Scd41 && equals(source, "i2c:0x62")) target = 0x62U;
  else if (driver == Driver::Ds3231 && equals(source, "i2c:0x68")) target = 0x68U;
  else if (driver == Driver::Rf433BinarySocket && equals(source, "socket:1")) target = 1U;
  else if (driver == Driver::Rf433BinarySocket && equals(source, "socket:2")) target = 2U;
  else if (driver == Driver::Rf433BinarySocket && equals(source, "socket:3")) target = 3U;
  else return false;
  return true;
}

template <std::size_t N>
class IdTable final {
public:
  bool add(const char* name, std::uint16_t assigned) noexcept {
    if (!validIdentifier(name) || count_ >= N || find(name) != 0U) return false;
    const std::size_t length = std::strlen(name);
    std::memcpy(names_[count_].data(), name, length);
    names_[count_][length] = '\0';
    ids_[count_] = assigned;
    ++count_;
    return true;
  }
  std::uint16_t find(const char* name) const noexcept {
    if (name == nullptr) return 0U;
    for (std::size_t i = 0; i < count_; ++i) {
      if (std::strcmp(names_[i].data(), name) == 0) return ids_[i];
    }
    return 0U;
  }
private:
  std::array<std::array<char, 41U>, N> names_{};
  std::array<std::uint16_t, N> ids_{};
  std::size_t count_{0U};
};

struct DecodeTables {
  IdTable<kMaxDevices> devices{};
  IdTable<kMaxActuators> actuators{};
  IdTable<kMaxProcesses> processes{};
};

bool parseDevices(const cJSON* root, Graph& graph, DecodeTables& names) noexcept {
  int count = 0;
  if (!arraySize(root, "devices", static_cast<int>(kMaxDevices), count, true)) return false;
  const cJSON* array = item(root, "devices");
  constexpr const char* fields[]{"id", "label", "driver", "locator", "enabled"};
  for (int i = 0; i < count; ++i) {
    const cJSON* row = cJSON_GetArrayItem(array, i);
    if (!exactObject(row, fields) || !nonEmptyLabel(stringItem(row, "label"))) return false;
    Driver driver{};
    if (!enumValue(stringItem(row, "driver"), kDrivers, driver)) return false;
    std::uint32_t locator = 0U;
    if (!physicalLocator(driver, stringItem(row, "locator"), locator)) return false;
    bool enabled = false;
    if (!booleanItem(row, "enabled", enabled)) return false;
    const auto id = static_cast<std::uint16_t>(i + 1);
    if (!names.devices.add(stringItem(row, "id"), id)) return false;
    graph.devices[i] = {id, driver, locator, enabled};
  }
  graph.device_count = static_cast<std::uint8_t>(count);
  return true;
}

bool parseSignals(const cJSON* root, Graph& graph, const DecodeTables& names) noexcept {
  int count = 0;
  if (!arraySize(root, "signals", static_cast<int>(kMaxSignals), count, true)) return false;
  const cJSON* array = item(root, "signals");
  constexpr const char* fields[]{"role", "sources"};
  constexpr const char* source_fields[]{"device_id", "channel"};
  std::uint32_t seen_roles = 0U;
  for (int i = 0; i < count; ++i) {
    const cJSON* row = cJSON_GetArrayItem(array, i);
    if (!exactObject(row, fields)) return false;
    SignalRole role{};
    if (!enumValue(stringItem(row, "role"), kSignals, role)) return false;
    const auto bit = static_cast<std::uint32_t>(1U << static_cast<std::uint8_t>(role));
    if ((seen_roles & bit) != 0U) return false;
    seen_roles |= bit;
    int sources = 0;
    if (!arraySize(row, "sources", static_cast<int>(kMaxSourcesPerRole), sources)) return false;
    for (int j = 0; j < sources; ++j) {
      const cJSON* source = cJSON_GetArrayItem(item(row, "sources"), j);
      if (!exactObject(source, source_fields)) return false;
      const auto device = names.devices.find(stringItem(source, "device_id"));
      Channel channel{};
      if (device == 0U ||
          !enumValue(stringItem(source, "channel"), kChannels, channel) ||
          graph.signal_count >= kMaxSignalSources) return false;
      graph.signals[graph.signal_count++] = {
          role, device, channel, static_cast<std::uint8_t>(j)};
    }
  }
  return true;
}

bool parseActuators(const cJSON* root, Graph& graph, DecodeTables& names) noexcept {
  int count = 0;
  if (!arraySize(root, "actuators", static_cast<int>(kMaxActuators), count)) return false;
  constexpr const char* fields[]{"id", "device_id", "channel", "role", "safe_state", "min_on_ms", "min_off_ms"};
  const cJSON* array = item(root, "actuators");
  for (int i = 0; i < count; ++i) {
    const cJSON* row = cJSON_GetArrayItem(array, i);
    if (!exactObject(row, fields) || !equals(stringItem(row, "safe_state"), "off")) return false;
    const auto device = names.devices.find(stringItem(row, "device_id"));
    Channel channel{};
    OutputRole role{};
    std::uint32_t on_ms = 0U, off_ms = 0U;
    if (device == 0U || !enumValue(stringItem(row, "channel"), kChannels, channel) ||
        !enumValue(stringItem(row, "role"), kOutputs, role) ||
        !integerItem(row, "min_on_ms", 0U, 3600000U, on_ms) ||
        !integerItem(row, "min_off_ms", 0U, 3600000U, off_ms)) return false;
    const auto id = static_cast<std::uint16_t>(101 + i);
    if (!names.actuators.add(stringItem(row, "id"), id)) return false;
    graph.actuators[i] = {id, device, channel, role, true, on_ms, off_ms};
  }
  graph.actuator_count = static_cast<std::uint8_t>(count);
  return true;
}

bool parseProcesses(const cJSON* root, Graph& graph, DecodeTables& names) noexcept {
  int count = 0;
  if (!arraySize(root, "processes", static_cast<int>(kMaxProcesses), count, true)) return false;
  constexpr const char* fields[]{"id", "kind", "enabled"};
  const cJSON* array = item(root, "processes");
  for (int i = 0; i < count; ++i) {
    const cJSON* row = cJSON_GetArrayItem(array, i);
    if (!exactObject(row, fields)) return false;
    ProcessKind kind{};
    bool enabled = false;
    if (!enumValue(stringItem(row, "kind"), kProcesses, kind) ||
        !booleanItem(row, "enabled", enabled)) return false;
    const auto id = static_cast<std::uint16_t>(201 + i);
    if (!names.processes.add(stringItem(row, "id"), id)) return false;
    graph.processes[i] = {id, kind, enabled};
  }
  graph.process_count = static_cast<std::uint8_t>(count);
  return true;
}

bool parseLinks(const cJSON* root, Graph& graph, const DecodeTables& names) noexcept {
  int count = 0;
  if (!arraySize(root, "links", static_cast<int>(kMaxLinks), count)) return false;
  constexpr const char* fields[]{"process_id", "actuator_id", "effect"};
  const cJSON* array = item(root, "links");
  for (int i = 0; i < count; ++i) {
    const cJSON* row = cJSON_GetArrayItem(array, i);
    if (!exactObject(row, fields)) return false;
    const auto process = names.processes.find(stringItem(row, "process_id"));
    const auto actuator = names.actuators.find(stringItem(row, "actuator_id"));
    Effect effect{};
    if (process == 0U || actuator == 0U ||
        !enumValue(stringItem(row, "effect"), kEffects, effect)) return false;
    graph.links[i] = {process, actuator, effect};
  }
  graph.link_count = static_cast<std::uint8_t>(count);
  return true;
}

bool parseTargets(const cJSON* node, RuntimeTargetSet& target) noexcept {
  constexpr const char* fields[]{"air_temperature_c", "relative_humidity_pct", "air_vpd_kpa"};
  return exactObject(node, fields) &&
         floatItem(node, "air_temperature_c", target.temperature_c) &&
         floatItem(node, "relative_humidity_pct", target.relative_humidity_pct) &&
         floatItem(node, "air_vpd_kpa", target.air_vpd_kpa);
}
bool parseControl(const cJSON* root, RuntimeScheduleProfile& config) noexcept {
  const cJSON* control = item(root, "control");
  constexpr const char* fields[]{"humidity_mode", "sensor_timeout_ms", "schedule", "targets"};
  constexpr const char* schedule_fields[]{"day_start_hour", "night_start_hour"};
  constexpr const char* target_fields[]{"day", "night"};
  if (!exactObject(control, fields)) return false;
  const char* mode = stringItem(control, "humidity_mode");
  if (equals(mode, "RH")) config.humidity_mode = ::growbox::climate::HumidityControlMode::Rh;
  else if (equals(mode, "VPD")) config.humidity_mode = ::growbox::climate::HumidityControlMode::Vpd;
  else return false;
  std::uint32_t timeout = 0U;
  if (!integerItem(control, "sensor_timeout_ms", 5000U, 300000U, timeout)) return false;
  config.sensor_timeout_ms = timeout;
  const cJSON* schedule = item(control, "schedule");
  if (!exactObject(schedule, schedule_fields)) return false;
  std::uint32_t day_hour = 0U, night_hour = 0U;
  if (!integerItem(schedule, "day_start_hour", 0U, 23U, day_hour) ||
      !integerItem(schedule, "night_start_hour", 0U, 23U, night_hour)) return false;
  config.day_start_hour = static_cast<std::uint8_t>(day_hour);
  config.night_start_hour = static_cast<std::uint8_t>(night_hour);
  const cJSON* targets = item(control, "targets");
  return exactObject(targets, target_fields) &&
         parseTargets(item(targets, "day"), config.day) &&
         parseTargets(item(targets, "night"), config.night) &&
         validateRuntimeScheduleProfile(config);
}

} // namespace

IpoJsonDecodeResult decodeIpoDeviceProfileJson(
    const char* bytes, std::size_t byte_count,
    std::uint32_t target_generation) noexcept {
  IpoJsonDecodeResult result{};
  result.status = IpoJsonStatus::InvalidLength;
  if (bytes == nullptr || byte_count == 0U ||
      byte_count > kMaximumIpoProfileJsonBytes || target_generation == 0U)
    return result;
  for (std::size_t i = 0; i < byte_count; ++i) {
    if (bytes[i] == '\0') return result;
  }

  const char* parse_end = nullptr;
  cJSON* root = cJSON_ParseWithLengthOpts(bytes, byte_count, &parse_end, false);
  struct JsonOwner {
    cJSON* root;
    ~JsonOwner() noexcept { cJSON_Delete(root); }
  } owner{root};
  result.status = IpoJsonStatus::InvalidJson;
  if (root == nullptr || parse_end == nullptr || parse_end < bytes ||
      parse_end > bytes + byte_count) return result;
  for (const char* p = parse_end; p < bytes + byte_count; ++p) {
    if (*p != ' ' && *p != '\n' && *p != '\r' && *p != '\t') return result;
  }
  constexpr const char* root_fields[]{
    "kind", "schema_version", "deployment_status", "profile_id",
    "board", "devices", "signals", "actuators", "processes", "links", "control"
  };
  result.status = IpoJsonStatus::InvalidFields;
  if (!exactObject(root, root_fields)) return result;

  std::uint32_t version = 0U;
  result.status = IpoJsonStatus::UnsupportedSchema;
  if (!equals(stringItem(root, "kind"), "bloomml.device-profile") ||
      !integerItem(root, "schema_version", 2U, 2U, version) ||
      !equals(stringItem(root, "deployment_status"), "draft_not_applied") ||
      !validIdentifier(stringItem(root, "profile_id")) ||
      !equals(stringItem(root, "board"), "crowpanel-esp32s3-2_9-n8r8"))
    return result;

  DurableIpoProfile candidate{};
  candidate.generation = target_generation;
  DecodeTables tables{};
  result.status = IpoJsonStatus::InvalidReference;
  if (!parseDevices(root, candidate.graph, tables) ||
      !parseSignals(root, candidate.graph, tables) ||
      !parseActuators(root, candidate.graph, tables) ||
      !parseProcesses(root, candidate.graph, tables) ||
      !parseLinks(root, candidate.graph, tables) ||
      !parseControl(root, candidate.schedule))
    return result;

  const auto admission = validateDurableIpoProfile(candidate);
  result.status = admission == IpoProfileCodecStatus::InvalidGraph
                      ? IpoJsonStatus::InvalidGraph
                      : IpoJsonStatus::UnsupportedBoard;
  if (admission != IpoProfileCodecStatus::Ok) return result;
  result.profile = candidate;
  result.status = IpoJsonStatus::Ok;
  result.admitted = true; // storage candidate, not physical output authority
  return result;
}

} // namespace growbox::app::climate_io::runtime::profile
