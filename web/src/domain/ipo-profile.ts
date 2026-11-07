import exampleV2 from "../../../profiles/crowpanel-ipo.v2.json"
import {
  DRIVER_CATALOG, DRIVER_IDS, EFFECT_ROLE, PROCESS_CATALOG, SIGNAL_ROLES,
  locatorMatches, validDriver, validEffect, validOutputRole, validProcessKind, validSignalRole,
} from "./ipo-catalog"
import type { DriverId, Effect, OutputRole, ProcessKind, SignalRole } from "./ipo-catalog"
import { validateDeviceProfile as validateLegacyV1 } from "./device-profile"
import type { DeviceProfile as LegacyV1Profile } from "./device-profile"

export type Device = { id: string; label: string; driver: DriverId; locator: string; enabled: boolean }
export type SignalSource = { device_id: string; channel: string }
export type SignalBinding = { role: SignalRole; sources: SignalSource[] }
export type Actuator = {
  id: string; device_id: string; channel: string; role: OutputRole;
  safe_state: "off"; min_on_ms: number; min_off_ms: number
}
export type Process = { id: string; kind: ProcessKind; enabled: boolean }
export type ProcessLink = { process_id: string; actuator_id: string; effect: Effect }
export type Targets = { air_temperature_c: number; relative_humidity_pct: number; air_vpd_kpa: number }
export interface IpoProfileV2 {
  kind: "bloomml.device-profile"
  schema_version: 2
  deployment_status: "draft_not_applied"
  profile_id: string
  board: string
  devices: Device[]
  signals: SignalBinding[]
  actuators: Actuator[]
  processes: Process[]
  links: ProcessLink[]
  control: {
    humidity_mode: "RH" | "VPD"
    sensor_timeout_ms: number
    schedule: { day_start_hour: number; night_start_hour: number }
    targets: { day: Targets; night: Targets }
  }
}
export const MAX_IPO_DEVICES = 24
export const MAX_IPO_SIGNALS = 16
export const MAX_IPO_ACTUATORS = 24
export const MAX_IPO_PROCESSES = 16
export const MAX_IPO_LINKS = 48
const idRegex = /^[a-z][a-z0-9_-]{1,39}$/

export function createDefaultIpoProfile(): IpoProfileV2 {
  return structuredClone(exampleV2) as unknown as IpoProfileV2
}

function record(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null && !Array.isArray(value)
}
function keys(
  value: unknown, path: string, fields: readonly string[], errors: string[],
): Record<string, unknown> | null {
  if (!record(value)) {
    errors.push(path + ": wymagany obiekt")
    return null
  }
  for (const name of fields) {
    if (!Object.prototype.hasOwnProperty.call(value, name)) errors.push(path + ": brak " + name)
  }
  for (const name of Object.keys(value)) {
    if (!fields.includes(name)) errors.push(path + ": nieznane pole " + name)
  }
  return value
}
function list(value: unknown, path: string, max: number, errors: string[]): unknown[] {
  if (!Array.isArray(value) || value.length > max) {
    errors.push(path + ": wymagana lista z maksymalnie " + max + " elementami")
    return []
  }
  return value
}
function identifier(value: unknown, path: string, errors: string[]): void {
  if (typeof value !== "string" || !idRegex.test(value)) {
    errors.push(path + ": ID 2–40 znaków (małe litery, cyfry, _ i -)")
  }
}
function numberRange(
  value: unknown, path: string, min: number, max: number, errors: string[], integer = false,
): void {
  if (typeof value !== "number" || !Number.isFinite(value) || value < min || value > max ||
      (integer && !Number.isInteger(value))) {
    errors.push(path + ": liczba " + min + "–" + max + (integer ? " (całkowita)" : ""))
  }
}
function unique(value: unknown, seen: Set<string>, path: string, errors: string[]): void {
  if (typeof value !== "string") return
  if (seen.has(value)) errors.push(path + ": duplikat " + value)
  seen.add(value)
}

/** Strict DTO + graph validator. No eval, implicit driver detection, arbitrary command or output owner. */
export function validateIpoProfile(value: unknown): string[] {
  const errors: string[] = []
  const root = keys(value, "profile",
    ["kind", "schema_version", "deployment_status", "profile_id", "board", "devices", "signals", "actuators", "processes", "links", "control"], errors)
  if (!root) return errors
  if (root.kind !== "bloomml.device-profile" || root.schema_version !== 2 ||
      root.deployment_status !== "draft_not_applied") {
    errors.push("profile: nieobsługiwana wersja/rodzaj/status")
  }
  identifier(root.profile_id, "profile_id", errors)
  if (typeof root.board !== "string" || root.board.length < 2 || root.board.length > 80) {
    errors.push("board: niepoprawna nazwa")
  }
  const devices = list(root.devices, "devices", MAX_IPO_DEVICES, errors)
  if (devices.length === 0) errors.push("devices: wymagane przynajmniej jedno urządzenie")
  const deviceSeen = new Set<string>()
  const locators = new Set<string>()
  const deviceMap = new Map<string, { driver: DriverId; enabled: boolean }>()
  for (const [i, item] of devices.entries()) {
    const p = "devices[" + i + "]"
    const d = keys(item, p, ["id", "label", "driver", "locator", "enabled"], errors)
    if (!d) continue
    identifier(d.id, p + ".id", errors)
    unique(d.id, deviceSeen, p + ".id", errors)
    if (typeof d.label !== "string" || !d.label.trim() || d.label.length > 80) {
      errors.push(p + ".label: wymagane 1–80 znaków")
    }
    if (!validDriver(d.driver)) {
      errors.push(p + ".driver: brak drivera w katalogu")
    }
    if (typeof d.enabled !== "boolean") errors.push(p + ".enabled: wymagany boolean")
    if (typeof d.locator !== "string" || d.locator.length > 80 ||
        (validDriver(d.driver) && !locatorMatches(d.driver, d.locator))) {
      errors.push(p + ".locator: niepoprawny adres dla wybranego drivera")
    }
    if (validDriver(d.driver) && typeof d.id === "string" && typeof d.enabled === "boolean") {
      deviceMap.set(d.id, { driver: d.driver, enabled: d.enabled })
      if (d.enabled && typeof d.locator === "string") {
        const endpoint = DRIVER_CATALOG[d.driver].transport + ":" + d.locator
        unique(endpoint, locators, p + ".locator", errors)
      }
    }
  }
  const signals = list(root.signals, "signals", MAX_IPO_SIGNALS, errors)
  if (signals.length === 0) errors.push("signals: wymagane przynajmniej jedno przypisanie")
  const signalSeen = new Set<string>()
  const assignedSignals = new Map<SignalRole, number>()
  for (const [i, item] of signals.entries()) {
    const p = "signals[" + i + "]"
    const s = keys(item, p, ["role", "sources"], errors)
    if (!s) continue
    if (!validSignalRole(s.role)) {
      errors.push(p + ".role: nieznana rola sygnału")
      continue
    }
    unique(s.role, signalSeen, p + ".role", errors)
    const sources = list(s.sources, p + ".sources", 4, errors)
    assignedSignals.set(s.role, sources.length)
    const sourceSeen = new Set<string>()
    for (const [j, itemSource] of sources.entries()) {
      const q = p + ".sources[" + j + "]"
      const source = keys(itemSource, q, ["device_id", "channel"], errors)
      if (!source) continue
      identifier(source.device_id, q + ".device_id", errors)
      if (typeof source.channel !== "string" || !source.channel) {
        errors.push(q + ".channel: wymagany kanał")
        continue
      }
      unique(String(source.device_id) + ":" + source.channel, sourceSeen, q, errors)
      const device = deviceMap.get(String(source.device_id))
      if (!device) {
        errors.push(q + ": urządzenie wejściowe nie istnieje")
        continue
      }
      const channel = DRIVER_CATALOG[device.driver].channels.find(
        (c) => c.id === source.channel && c.direction === "input",
      )
      if (!channel || channel.unit !== SIGNAL_ROLES[s.role].unit) {
        errors.push(q + ": kanał/typ/jednostka niezgodne z rolą " + s.role)
      }
    }
  }
  const actuators = list(root.actuators, "actuators", MAX_IPO_ACTUATORS, errors)
  const actuatorSeen = new Set<string>()
  const endpointSeen = new Set<string>()
  const actuatorMap = new Map<string, { role: OutputRole; device_id: string }>()
  for (const [i, item] of actuators.entries()) {
    const p = "actuators[" + i + "]"
    const a = keys(item, p, ["id", "device_id", "channel", "role", "safe_state", "min_on_ms", "min_off_ms"], errors)
    if (!a) continue
    identifier(a.id, p + ".id", errors)
    identifier(a.device_id, p + ".device_id", errors)
    unique(a.id, actuatorSeen, p + ".id", errors)
    if (!validOutputRole(a.role)) errors.push(p + ".role: nieznana funkcja urządzenia")
    if (a.safe_state !== "off") errors.push(p + ".safe_state: wymagane bezpieczne OFF")
    numberRange(a.min_on_ms, p + ".min_on_ms", 0, 3600000, errors, true)
    numberRange(a.min_off_ms, p + ".min_off_ms", 0, 3600000, errors, true)
    if (typeof a.channel !== "string" || !a.channel) errors.push(p + ".channel: wymagany kanał")
    const device = deviceMap.get(String(a.device_id))
    if (!device) {
      errors.push(p + ": urządzenie wyjściowe nie istnieje")
    } else {
      const channel = DRIVER_CATALOG[device.driver].channels.find(
        (c) => c.id === a.channel && c.direction === "output",
      )
      if (!channel) errors.push(p + ": wybrany driver nie udostępnia wyjścia " + String(a.channel))
    }
    unique(String(a.device_id) + ":" + String(a.channel), endpointSeen, p + ": fizyczny kanał", errors)
    if (typeof a.id === "string" && validOutputRole(a.role) && typeof a.device_id === "string") {
      actuatorMap.set(a.id, { role: a.role, device_id: a.device_id })
    }
  }
  const processes = list(root.processes, "processes", MAX_IPO_PROCESSES, errors)
  if (processes.length === 0) errors.push("processes: wymagany proces")
  const processSeen = new Set<string>()
  const processMap = new Map<string, { kind: ProcessKind; enabled: boolean }>()
  let co2SafetyCount = 0
  for (const [i, item] of processes.entries()) {
    const p = "processes[" + i + "]"
    const x = keys(item, p, ["id", "kind", "enabled"], errors)
    if (!x) continue
    identifier(x.id, p + ".id", errors)
    unique(x.id, processSeen, p + ".id", errors)
    if (!validProcessKind(x.kind)) errors.push(p + ".kind: nieznany proces")
    if (typeof x.enabled !== "boolean") errors.push(p + ".enabled: wymagany boolean")
    if (x.kind === "co2_safety") {
      co2SafetyCount += 1
      if (co2SafetyCount > 1) errors.push("processes: tylko jeden chroniony proces co2_safety")
      if (x.enabled !== true) errors.push(p + ": ochrona CO₂ nie może być wyłączona w profilu")
    }
    if (typeof x.id === "string" && validProcessKind(x.kind) && typeof x.enabled === "boolean") {
      processMap.set(x.id, { kind: x.kind, enabled: x.enabled })
    }
  }
  if (co2SafetyCount === 0) errors.push("processes: brakuje chronionej roli co2_safety")
  const links = list(root.links, "links", MAX_IPO_LINKS, errors)
  const linkSeen = new Set<string>()
  for (const [i, item] of links.entries()) {
    const p = "links[" + i + "]"
    const link = keys(item, p, ["process_id", "actuator_id", "effect"], errors)
    if (!link) continue
    identifier(link.process_id, p + ".process_id", errors)
    identifier(link.actuator_id, p + ".actuator_id", errors)
    unique(String(link.process_id) + ":" + String(link.actuator_id) + ":" + String(link.effect),
      linkSeen, p, errors)
    const process = processMap.get(String(link.process_id))
    const actuator = actuatorMap.get(String(link.actuator_id))
    if (!process) errors.push(p + ": proces nie istnieje")
    if (!actuator) errors.push(p + ": wyjście nie istnieje")
    if (!validEffect(link.effect)) {
      errors.push(p + ".effect: nieznany efekt")
    } else {
      if (process && !PROCESS_CATALOG[process.kind].effects.includes(link.effect)) {
        errors.push(p + ": proces nie może wykonywać efektu " + link.effect)
      }
      if (actuator && EFFECT_ROLE[link.effect] !== actuator.role) {
        errors.push(p + ": efekt " + link.effect + " nie pasuje do funkcji wyjścia " + actuator.role)
      }
    }
  }
  const control = keys(root.control, "control",
    ["humidity_mode", "sensor_timeout_ms", "schedule", "targets"], errors)
  if (control) {
    if (control.humidity_mode !== "RH" && control.humidity_mode !== "VPD") {
      errors.push("control.humidity_mode: RH albo VPD")
    }
    numberRange(control.sensor_timeout_ms, "sensor_timeout_ms", 5000, 300000, errors, true)
    const schedule = keys(control.schedule, "control.schedule", ["day_start_hour", "night_start_hour"], errors)
    if (schedule) {
      numberRange(schedule.day_start_hour, "day_start_hour", 0, 23, errors, true)
      numberRange(schedule.night_start_hour, "night_start_hour", 0, 23, errors, true)
      if (schedule.day_start_hour === schedule.night_start_hour) errors.push("schedule: dzień i noc muszą się różnić")
    }
    const targets = keys(control.targets, "control.targets", ["day", "night"], errors)
    if (targets) {
      for (const period of ["day", "night"]) {
        const t = keys(targets[period], "targets." + period,
          ["air_temperature_c", "relative_humidity_pct", "air_vpd_kpa"], errors)
        if (t) {
          numberRange(t.air_temperature_c, period + ".temperature", 10, 40, errors)
          numberRange(t.relative_humidity_pct, period + ".humidity", 20, 90, errors)
          numberRange(t.air_vpd_kpa, period + ".vpd", 0.2, 3.5, errors)
        }
      }
    }
  }
  return errors
}

export function assessIpoProfile(profile: IpoProfileV2): string[] {
  const warnings: string[] = []
  const devices = new Map(profile.devices.map((d) => [d.id, d]))
  for (const device of profile.devices) {
    if (device.enabled && DRIVER_CATALOG[device.driver].status === "proposed") {
      warnings.push(device.label + ": driver projektowany; firmware nie ma jeszcze adaptera.")
    }
    if (device.locator.startsWith("runtime:")) {
      warnings.push(device.label + ": adres jest aliasem builda, nie dynamiczną identyfikacją sprzętu.")
    }
  }
  for (const signal of profile.signals) {
    if (!signal.sources.some((s) => devices.get(s.device_id)?.enabled)) {
      warnings.push(SIGNAL_ROLES[signal.role].label + ": brak aktywnego źródła pomiaru.")
    }
    if (signal.sources.some((s) => devices.get(s.device_id)?.driver === "i2c.scd41" &&
        (s.channel === "temperature_c" || s.channel === "humidity_pct"))) {
      warnings.push("SCD41 T/RH: kanał istnieje w sensorze, lecz firmware dziś używa z SCD41 tylko CO₂.")
    }
  }
  for (const proc of profile.processes) {
    if (!proc.enabled) continue
    const def = PROCESS_CATALOG[proc.kind]
    if (def.status === "proposed") warnings.push(def.label + ": proces projektowany, bez implementacji runtime.")
    for (const role of def.required) {
      const binding = profile.signals.find((s) => s.role === role)
      if (!binding?.sources.some((s) => devices.get(s.device_id)?.enabled)) {
        warnings.push(def.label + ": brakuje wejścia " + SIGNAL_ROLES[role].label)
      }
    }
  }
  const currentFirmwareRoles: Partial<Record<OutputRole, string>> = {
    exhaust_fan: "socket:1", light: "socket:2", humidifier: "socket:3",
  }
  for (const output of profile.actuators) {
    const device = devices.get(output.device_id)
    const requiredLocator = currentFirmwareRoles[output.role]
    if (!requiredLocator || device?.driver !== "rf433.binary_socket" ||
        device.locator !== requiredLocator || output.channel !== "switch") {
      warnings.push(output.id + ": poprawna definicja v2, ale brak takiego powiązania w obecnym firmware Mint.")
    }
    if ((output.role === "exhaust_fan" && (output.min_on_ms < 120000 || output.min_off_ms < 120000)) ||
        (output.role === "humidifier" && (output.min_on_ms < 180000 || output.min_off_ms < 180000))) {
      warnings.push(output.id + ": projektowe minima ON/OFF są niższe od aktualnej polityki firmware.")
    }
    if (!devices.get(output.device_id)?.enabled) {
      warnings.push(output.id + ": powiązane wyjście fizyczne jest wyłączone.")
    }
  }
  if (profile.links.length === 0) warnings.push("Brak połączeń procesów z wyjściami.")
  warnings.push("Mapa v2 jest szkicem. Obecne firmware nadal ma stałe role RF433 i Mint; brak NVS/apply/readback v2.")
  return [...new Set(warnings)]
}

export type IpoParseResult =
  | { success: true; profile: IpoProfileV2; migrated: boolean }
  | { success: false; errors: string[] }

export function migrateV1ToV2(value: LegacyV1Profile): IpoProfileV2 {
  const result = createDefaultIpoProfile()
  result.profile_id = value.profile_id
  result.control = structuredClone(value.control)
  const pairs: { v1: string; v2: string }[] = [
    { v1: "tp357_inside", v2: "tp357" },
    { v1: "scd41_inside", v2: "scd41" },
    { v1: "xiaomi_nearby", v2: "xiaomi" },
    { v1: "ds3231_clock", v2: "rtc" },
  ]
  for (const pair of pairs) {
    const old = value.hardware.sensors[pair.v1 as keyof LegacyV1Profile["hardware"]["sensors"]]
    const target = result.devices.find((d) => d.id === pair.v2)
    if (target && old) target.enabled = old.installed
  }
  for (const [role, id] of [
    ["exhaust_fan", "fan_socket"], ["scheduled_light", "lamp_socket"],
    ["humidifier", "humidifier_socket"],
  ] as const) {
    const target = result.devices.find((d) => d.id === id)
    if (target) target.enabled = value.hardware.outputs[role].installed
  }
  return result
}

export function parseIpoJson(raw: string): IpoParseResult {
  let data: unknown
  try { data = JSON.parse(raw) as unknown }
  catch { return { success: false, errors: ["Niepoprawny JSON."] } }
  if (record(data) && data.kind === "bloomml.device-profile" && data.schema_version === 1) {
    const errors = validateLegacyV1(data)
    if (errors.length) return { success: false, errors }
    const profile = migrateV1ToV2(data as unknown as LegacyV1Profile)
    const migratedErrors = validateIpoProfile(profile)
    if (migratedErrors.length) return { success: false, errors: migratedErrors }
    return { success: true, profile, migrated: true }
  }
  const errors = validateIpoProfile(data)
  if (errors.length) return { success: false, errors }
  return { success: true, profile: data as IpoProfileV2, migrated: false }
}

/** Semantic preview ONLY. These are never RF/GPIO commands and no code here has a transport. */
export type ProposedIntent = { process_id: string; actuator_id: string; effect: Effect; level: number }
export type SymbolicDecision = {
  actuator_id: string; level: number; contributors: string[]; status: "symbolic_preview"
}
export function previewOutputArbitration(
  profile: IpoProfileV2,
  proposals: readonly ProposedIntent[],
): { decisions: SymbolicDecision[]; rejected: string[] } {
  const rejected: string[] = []
  if (validateIpoProfile(profile).length) {
    return { decisions: [], rejected: ["Niepoprawny profil: podgląd arbitrażu zablokowany."] }
  }
  const processes = new Map(profile.processes.map((p) => [p.id, p]))
  const actuators = new Map(profile.actuators.map((a) => [a.id, a]))
  const devices = new Map(profile.devices.map((d) => [d.id, d]))
  const decisions = new Map<string, SymbolicDecision>()
  for (const proposal of proposals) {
    const process = processes.get(proposal.process_id)
    const actuator = actuators.get(proposal.actuator_id)
    const linked = profile.links.some((l) => l.process_id === proposal.process_id &&
      l.actuator_id === proposal.actuator_id && l.effect === proposal.effect)
    if (!process?.enabled || PROCESS_CATALOG[process.kind].protected ||
        !actuator || !devices.get(actuator.device_id)?.enabled ||
        !linked || !Number.isFinite(proposal.level) || proposal.level < 0 || proposal.level > 1) {
      rejected.push(proposal.process_id + "→" + proposal.actuator_id + ": niezgodna/nieaktywna propozycja")
      continue
    }
    const current = decisions.get(actuator.id) ?? {
      actuator_id: actuator.id, level: 0, contributors: [], status: "symbolic_preview" as const,
    }
    current.level = Math.max(current.level, proposal.level)
    current.contributors.push(proposal.process_id)
    decisions.set(actuator.id, current)
  }
  return { decisions: [...decisions.values()], rejected }
}

export function createIpoDevice(id: string, driver: DriverId): Device {
  const definitions = DRIVER_CATALOG[driver]
  const preset: Record<DriverId, string> = {
    "ble.tp357": "runtime:tp357", "ble.xiaomi": "runtime:xiaomi",
    "i2c.scd41": "i2c:0x62", "i2c.ds3231": "i2c:0x68",
    "i2c.bme280": "i2c:0x76", "i2c.bh1750": "i2c:0x23",
    "adc.soil": "adc:0", "rf433.binary_socket": "socket:4",
    "gpio.relay": "gpio:4", "pwm.channel": "pwm:0", "modbus.relay": "modbus:1:1",
  }
  return { id, label: definitions.label, driver, locator: preset[driver], enabled: true }
}

export function outputChannels(driver: DriverId): string[] {
  return DRIVER_CATALOG[driver].channels.filter((c) => c.direction === "output").map((c) => c.id)
}
export function inputChannels(driver: DriverId, role: SignalRole): string[] {
  return DRIVER_CATALOG[driver].channels.filter((c) =>
    c.direction === "input" && c.unit === SIGNAL_ROLES[role].unit,
  ).map((c) => c.id)
}
export { DRIVER_IDS }
