/**
 * Preview of the CURRENT, narrow CrowPanel firmware admission gate.
 * This is NOT hardware detection, attestation, qualification or permission
 * to apply anything. Future drivers remain expressible in the v2 schema.
 *
 * Keep this diagnostic aligned with src/climate/runtime/profile/IpoProfileJsonDecode.cpp
 * (current locator normalization) and lib/ipo_profile/src/IpoBoardAdmission.h
 * (current reference-only graph admission).
 */
import { validateIpoProfile } from "./ipo-profile"
import type { IpoProfileV2 } from "./ipo-profile"
import type { OutputRole } from "./ipo-catalog"

export const MAX_FIRMWARE_IPO_JSON_BYTES = 32768
export const CROWPANEL_BOARD_ID = "crowpanel-esp32s3-2_9-n8r8"

export type CrowPanelProfileReadiness = {
  structurallyValid: boolean
  matchesCurrentFirmware: boolean
  applyEnabled: false
  jsonBytes: number
  reasons: string[]
}

type DeviceDescriptor = { driver: string; locator: string }

const acceptedLocators: Readonly<Record<string, readonly string[]>> = {
  "ble.tp357": ["runtime:tp357"],
  "ble.xiaomi": ["runtime:xiaomi"],
  "i2c.scd41": ["i2c:0x62"],
  "i2c.ds3231": ["i2c:0x68"],
  "rf433.binary_socket": ["socket:1", "socket:2", "socket:3"],
}

const fixedOutputBindings: Readonly<Record<string, {
  role: OutputRole
  driver: string
  locator: string
  minDwell: number
}>> = {
  exhaust_fan: { role: "exhaust_fan", driver: "rf433.binary_socket", locator: "socket:1", minDwell: 120000 },
  light: { role: "light", driver: "rf433.binary_socket", locator: "socket:2", minDwell: 0 },
  humidifier: { role: "humidifier", driver: "rf433.binary_socket", locator: "socket:3", minDwell: 180000 },
}

const requiredInputs: ReadonlyArray<{
  role: string
  driver: string
  channel: string
}> = [
  { role: "inside.temperature_c", driver: "ble.tp357", channel: "temperature_c" },
  { role: "inside.humidity_pct", driver: "ble.tp357", channel: "humidity_pct" },
  { role: "inside.co2_ppm", driver: "i2c.scd41", channel: "co2_ppm" },
  { role: "clock.time", driver: "i2c.ds3231", channel: "unix_time" },
]

const allowedInputs: Readonly<Record<string, { driver: string; channel: string }>> = {
  "inside.temperature_c": { driver: "ble.tp357", channel: "temperature_c" },
  "inside.humidity_pct": { driver: "ble.tp357", channel: "humidity_pct" },
  "inside.co2_ppm": { driver: "i2c.scd41", channel: "co2_ppm" },
  "outside.temperature_c": { driver: "ble.xiaomi", channel: "temperature_c" },
  "outside.humidity_pct": { driver: "ble.xiaomi", channel: "humidity_pct" },
  "clock.time": { driver: "i2c.ds3231", channel: "unix_time" },
}

const allowedProcesses = new Set(["temperature", "humidity", "co2_safety", "light_schedule"])
const requiredRoutes: ReadonlyArray<{ process: string; role: OutputRole; effect: string }> = [
  { process: "temperature", role: "exhaust_fan", effect: "ventilate" },
  { process: "humidity", role: "exhaust_fan", effect: "ventilate" },
  { process: "humidity", role: "humidifier", effect: "humidify" },
  { process: "co2_safety", role: "exhaust_fan", effect: "ventilate" },
  { process: "light_schedule", role: "light", effect: "illuminate" },
]

/** Use compact UTF-8 JSON: an export must fit the bounded device-side parser. */
export function profileJsonBytes(profile: IpoProfileV2): number {
  return new TextEncoder().encode(JSON.stringify(profile)).length
}

export function assessCrowPanelProfile(profile: IpoProfileV2): CrowPanelProfileReadiness {
  const errors = validateIpoProfile(profile)
  const reasons: string[] = []
  const jsonBytes = profileJsonBytes(profile)
  if (errors.length) {
    reasons.push("Struktura v2 zawiera błędy — najpierw popraw walidację formularza.")
    return {
      structurallyValid: false, matchesCurrentFirmware: false, applyEnabled: false,
      jsonBytes, reasons,
    }
  }

  if (profile.board !== CROWPANEL_BOARD_ID) {
    reasons.push("Profil wskazuje inną płytkę niż obecny CrowPanel.")
  }
  if (jsonBytes > MAX_FIRMWARE_IPO_JSON_BYTES) {
    reasons.push("Profil przekracza limit " + MAX_FIRMWARE_IPO_JSON_BYTES + " bajtów dla parsera firmware.")
  }

  const devices = new Map(profile.devices.map((d) => [d.id, d]))
  for (const device of profile.devices) {
    const locators = acceptedLocators[device.driver]
    // The current firmware decoder rejects unknown locators even on a disabled device.
    if (!locators?.includes(device.locator)) {
      reasons.push(device.id + ": driver/adres nieobsługiwany przez aktualny firmware CrowPanel.")
    }
  }

  for (const required of requiredInputs) {
    const binding = profile.signals.find((s) => s.role === required.role)
    const primary = binding?.sources[0]
    const device = primary ? devices.get(primary.device_id) : undefined
    if (!device?.enabled || device.driver !== required.driver ||
        primary?.channel !== required.channel) {
      reasons.push(required.role + ": brak wymaganego aktywnego źródła głównego " + required.driver + ".")
    }
  }
  for (const signal of profile.signals) {
    const accepted = allowedInputs[signal.role]
    for (const [priority, source] of signal.sources.entries()) {
      const device = devices.get(source.device_id)
      if (!accepted || !device?.enabled || priority !== 0 ||
          device.driver !== accepted.driver || source.channel !== accepted.channel) {
        reasons.push(signal.role + ": nieobsługiwane źródło/fallback w aktualnym firmware.")
      }
    }
  }

  for (const process of profile.processes) {
    if (!allowedProcesses.has(process.kind)) {
      reasons.push(process.id + ": proces " + process.kind + " jest na razie projektowy.")
    }
  }

  if (profile.actuators.length !== 3) {
    reasons.push("Obecny firmware wymaga dokładnie trzech wyjść RF433: wyciąg, lampa, nawilżacz.")
  }
  const seenRoles = new Set<string>()
  for (const actuator of profile.actuators) {
    const expected = fixedOutputBindings[actuator.role]
    const device = devices.get(actuator.device_id)
    if (!expected || !device?.enabled || device.driver !== expected.driver ||
        device.locator !== expected.locator || actuator.channel !== "switch") {
      reasons.push(actuator.id + ": nieobsługiwane mapowanie fizyczne wyjścia.")
    }
    if (expected && (actuator.min_on_ms < expected.minDwell ||
                      actuator.min_off_ms < expected.minDwell)) {
      reasons.push(actuator.id + ": czasy ON/OFF poniżej polityki sprzętowej.")
    }
    if (seenRoles.has(actuator.role)) reasons.push(actuator.role + ": powtórzona rola wyjścia.")
    seenRoles.add(actuator.role)
  }
  for (const role of Object.keys(fixedOutputBindings)) {
    if (!seenRoles.has(role)) reasons.push(role + ": brak obowiązkowego wyjścia Mint.")
  }

  for (const route of requiredRoutes) {
    const present = profile.links.some((link) => {
      const process = profile.processes.find((p) => p.id === link.process_id)
      const actuator = profile.actuators.find((a) => a.id === link.actuator_id)
      return process?.enabled && process.kind === route.process &&
        actuator?.role === route.role && link.effect === route.effect
    })
    if (!present) {
      reasons.push(route.process + " → " + route.effect + " → " + route.role +
        ": brak wymaganego połączenia.")
    }
  }

  return {
    structurallyValid: true,
    matchesCurrentFirmware: reasons.length === 0,
    applyEnabled: false,
    jsonBytes,
    reasons: [...new Set(reasons)],
  }
}
