/** Catalog of declared capabilities, not permission to actuate hardware. Driver 'present'
 * means the existing firmware contains a driver, not that dynamic v2 registration is live. */
export const SIGNAL_ROLES = {
  "inside.temperature_c": { label: "Temperatura w komorze", unit: "degC" },
  "inside.humidity_pct": { label: "Wilgotność w komorze", unit: "percent" },
  "inside.co2_ppm": { label: "CO₂ w komorze", unit: "ppm" },
  "outside.temperature_c": { label: "Temperatura otoczenia", unit: "degC" },
  "outside.humidity_pct": { label: "Wilgotność otoczenia", unit: "percent" },
  "clock.time": { label: "Czas", unit: "unix_s" },
  "light.illuminance_lux": { label: "Natężenie oświetlenia (lux, NIE PPFD)", unit: "lux" },
  "substrate.moisture_pct": { label: "Wilgotność podłoża", unit: "percent" },
} as const
export type SignalRole = keyof typeof SIGNAL_ROLES

export type DriverId =
  | "ble.tp357" | "ble.xiaomi" | "i2c.scd41" | "i2c.ds3231"
  | "i2c.bme280" | "i2c.bh1750" | "adc.soil"
  | "rf433.binary_socket" | "gpio.relay" | "pwm.channel" | "modbus.relay"
export type Channel = { id: string; unit: string; direction: "input" | "output"; control?: "binary" | "continuous" }
export type DriverDefinition = {
  label: string
  transport: "ble" | "i2c" | "rf433" | "gpio" | "pwm" | "modbus" | "adc"
  status: "present_in_firmware" | "proposed"
  channels: readonly Channel[]
}

const input = (id: string, unit: string): Channel => ({ id, unit, direction: "input" })
const output = (id: string, control: "binary" | "continuous"): Channel => ({
  id, unit: "normalized", direction: "output", control,
})

export const DRIVER_CATALOG: Record<DriverId, DriverDefinition> = {
  "ble.tp357": { label: "TP357 BLE", transport: "ble", status: "present_in_firmware", channels: [input("temperature_c", "degC"), input("humidity_pct", "percent")] },
  "ble.xiaomi": { label: "Xiaomi BLE", transport: "ble", status: "present_in_firmware", channels: [input("temperature_c", "degC"), input("humidity_pct", "percent")] },
  "i2c.scd41": { label: "SCD41", transport: "i2c", status: "present_in_firmware", channels: [input("co2_ppm", "ppm"), input("temperature_c", "degC"), input("humidity_pct", "percent")] },
  "i2c.ds3231": { label: "DS3231", transport: "i2c", status: "present_in_firmware", channels: [input("unix_time", "unix_s")] },
  "i2c.bme280": { label: "BME280", transport: "i2c", status: "proposed", channels: [input("temperature_c", "degC"), input("humidity_pct", "percent")] },
  "i2c.bh1750": { label: "BH1750", transport: "i2c", status: "proposed", channels: [input("illuminance_lux", "lux")] },
  "adc.soil": { label: "Czujnik gleby ADC", transport: "adc", status: "proposed", channels: [input("moisture_pct", "percent")] },
  "rf433.binary_socket": { label: "Gniazdo RF433 ON/OFF", transport: "rf433", status: "present_in_firmware", channels: [output("switch", "binary")] },
  "gpio.relay": { label: "Przekaźnik GPIO", transport: "gpio", status: "proposed", channels: [output("switch", "binary")] },
  "pwm.channel": { label: "Wyjście PWM", transport: "pwm", status: "proposed", channels: [output("duty", "continuous")] },
  "modbus.relay": { label: "Przekaźnik Modbus", transport: "modbus", status: "proposed", channels: [output("switch", "binary")] },
}

export type OutputRole =
  | "exhaust_fan" | "humidifier" | "light" | "heater" | "cooler"
  | "dehumidifier" | "co2_doser" | "circulation_fan" | "irrigation_pump"
export const OUTPUT_ROLES: Record<OutputRole, string> = {
  exhaust_fan: "Wyciąg", humidifier: "Nawilżacz", light: "Lampa",
  heater: "Grzałka", cooler: "Chłodzenie", dehumidifier: "Osuszacz",
  co2_doser: "Dozownik CO₂", circulation_fan: "Obieg powietrza",
  irrigation_pump: "Pompa podlewania",
}
export type Effect =
  | "ventilate" | "humidify" | "illuminate" | "heat" | "cool"
  | "dehumidify" | "dose_co2" | "circulate" | "irrigate"
export const EFFECT_ROLE: Record<Effect, OutputRole> = {
  ventilate: "exhaust_fan", humidify: "humidifier", illuminate: "light",
  heat: "heater", cool: "cooler", dehumidify: "dehumidifier",
  dose_co2: "co2_doser", circulate: "circulation_fan", irrigate: "irrigation_pump",
}
export const EFFECT_LABELS: Record<Effect, string> = {
  ventilate: "Wietrzenie", humidify: "Nawilżanie", illuminate: "Oświetlenie",
  heat: "Grzanie", cool: "Chłodzenie", dehumidify: "Osuszanie",
  dose_co2: "Dozowanie CO₂", circulate: "Obieg powietrza", irrigate: "Podlewanie",
}

export type ProcessKind =
  | "temperature" | "humidity" | "co2_safety" | "co2_target"
  | "light_schedule" | "circulation" | "irrigation"
export type ProcessDefinition = {
  label: string
  required: readonly SignalRole[]
  effects: readonly Effect[]
  protected?: boolean
  status: "production_rule" | "firmware_safety" | "proposed"
}
export const PROCESS_CATALOG: Record<ProcessKind, ProcessDefinition> = {
  temperature: { label: "Regulacja temperatury", required: ["inside.temperature_c", "inside.humidity_pct"], effects: ["ventilate", "heat", "cool"], status: "production_rule" },
  humidity: { label: "Regulacja RH/VPD", required: ["inside.temperature_c", "inside.humidity_pct"], effects: ["ventilate", "humidify", "dehumidify"], status: "production_rule" },
  co2_safety: { label: "Nadrzędna ochrona CO₂", required: ["inside.co2_ppm"], effects: ["ventilate"], protected: true, status: "firmware_safety" },
  co2_target: { label: "Regulacja CO₂", required: ["inside.co2_ppm"], effects: ["dose_co2"], status: "proposed" },
  light_schedule: { label: "Harmonogram światła", required: ["clock.time"], effects: ["illuminate"], status: "production_rule" },
  circulation: { label: "Obieg powietrza", required: ["clock.time"], effects: ["circulate"], status: "proposed" },
  irrigation: { label: "Podlewanie", required: ["substrate.moisture_pct"], effects: ["irrigate"], status: "proposed" },
}

export const DRIVER_IDS = Object.keys(DRIVER_CATALOG) as DriverId[]
export const SIGNAL_ROLE_IDS = Object.keys(SIGNAL_ROLES) as SignalRole[]
export const OUTPUT_ROLE_IDS = Object.keys(OUTPUT_ROLES) as OutputRole[]
export const PROCESS_KIND_IDS = Object.keys(PROCESS_CATALOG) as ProcessKind[]
export const EFFECT_IDS = Object.keys(EFFECT_ROLE) as Effect[]

export function validDriver(id: unknown): id is DriverId {
  return typeof id === "string" && Object.prototype.hasOwnProperty.call(DRIVER_CATALOG, id)
}
export function validSignalRole(id: unknown): id is SignalRole {
  return typeof id === "string" && Object.prototype.hasOwnProperty.call(SIGNAL_ROLES, id)
}
export function validOutputRole(id: unknown): id is OutputRole {
  return typeof id === "string" && Object.prototype.hasOwnProperty.call(OUTPUT_ROLES, id)
}
export function validProcessKind(id: unknown): id is ProcessKind {
  return typeof id === "string" && Object.prototype.hasOwnProperty.call(PROCESS_CATALOG, id)
}
export function validEffect(id: unknown): id is Effect {
  return typeof id === "string" && Object.prototype.hasOwnProperty.call(EFFECT_ROLE, id)
}
export function locatorMatches(driver: DriverId, locator: string): boolean {
  const type = DRIVER_CATALOG[driver].transport
  if (type === "ble") return /^(runtime:(tp357|xiaomi)|ble:([0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2})$/.test(locator)
  if (type === "i2c") return /^i2c:0x[0-7][0-9a-fA-F]$/.test(locator)
  if (type === "rf433") return /^socket:([1-9]|1[0-6])$/.test(locator)
  if (type === "gpio") return /^gpio:([0-9]|[1-3][0-9]|4[0-8])$/.test(locator)
  if (type === "pwm") return /^pwm:([0-9]|1[0-5])$/.test(locator)
  if (type === "modbus") return /^modbus:([1-9]|[1-9][0-9]):([1-9][0-9]{0,4})$/.test(locator)
  return /^adc:([0-9]|1[0-5])$/.test(locator)
}
