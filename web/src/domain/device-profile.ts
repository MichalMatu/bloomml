import defaultProfile from "../../../profiles/crowpanel-mint.v1.json"

export const SENSOR_DEFINITIONS = [
  { id: "tp357_inside", label: "TP357 · wewnątrz", signals: "Temperatura, RH", role: "Sterowanie T/RH/VPD i bezpieczeństwo", required: true },
  { id: "scd41_inside", label: "SCD41 · wewnątrz", signals: "CO₂", role: "Monitoring CO₂; T/RH z SCD41 nie zastępują dziś TP357", required: false },
  { id: "xiaomi_nearby", label: "Xiaomi BLE · otoczenie", signals: "Temperatura, RH poza komorą", role: "Kontekst wymiany powietrza", required: false },
  { id: "ds3231_clock", label: "DS3231 · zegar", signals: "Czas lokalny", role: "Wybór trybu dzień/noc i harmonogram lampy", required: true },
] as const

export const OUTPUT_DEFINITIONS = [
  { id: "exhaust_fan", label: "Wyciąg", endpoint: "rf433_socket_1", description: "Wymiana powietrza: wpływ na T, RH/VPD i CO₂ zależnie od otoczenia", kind: "rule" },
  { id: "scheduled_light", label: "Lampa", endpoint: "rf433_socket_2", description: "Harmonogram dzień/noc + niezależny termiczny interlock; bez sterowania ML", kind: "schedule" },
  { id: "humidifier", label: "Nawilżacz", endpoint: "rf433_socket_3", description: "Podnosi RH / obniża VPD, z minimalnymi czasami pracy/przerwy", kind: "rule" },
] as const

export type SensorId = (typeof SENSOR_DEFINITIONS)[number]["id"]
export type OutputId = (typeof OUTPUT_DEFINITIONS)[number]["id"]
export type HumidityMode = "RH" | "VPD"
export type Period = "day" | "night"

export type EnvironmentTargets = {
  air_temperature_c: number
  relative_humidity_pct: number
  air_vpd_kpa: number
}

export interface DeviceProfile {
  kind: "bloomml.device-profile"
  schema_version: 1
  deployment_status: "draft_not_applied"
  profile_id: string
  hardware: {
    board: "crowpanel-esp32s3-2_9-n8r8"
    sensors: Record<SensorId, { installed: boolean }>
    outputs: Record<OutputId, { installed: boolean; endpoint: string }>
  }
  control: {
    humidity_mode: HumidityMode
    sensor_timeout_ms: number
    schedule: { day_start_hour: number; night_start_hour: number }
    targets: Record<Period, EnvironmentTargets>
  }
}

export function createDefaultDeviceProfile(): DeviceProfile {
  return structuredClone(defaultProfile) as DeviceProfile
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return value !== null && typeof value === "object" && !Array.isArray(value)
}

function objectAt(
  value: unknown,
  path: string,
  keys: readonly string[],
  errors: string[],
): Record<string, unknown> | null {
  if (!isRecord(value)) {
    errors.push(path + ": wymagany obiekt")
    return null
  }
  for (const key of keys) {
    if (!Object.prototype.hasOwnProperty.call(value, key)) {
      errors.push(path + "." + key + ": brak pola")
    }
  }
  for (const key of Object.keys(value)) {
    if (!keys.includes(key)) errors.push(path + "." + key + ": nieznane pole")
  }
  return value
}

function numberAt(
  value: unknown,
  path: string,
  min: number,
  max: number,
  errors: string[],
  integer = false,
): void {
  if (
    typeof value !== "number" ||
    !Number.isFinite(value) ||
    (integer && !Number.isInteger(value)) ||
    value < min ||
    value > max
  ) {
    errors.push(path + ": dozwolony zakres " + min + "–" + max + (integer ? " (liczba całkowita)" : ""))
  }
}

/** Reject unknown keys and invented endpoints as well as out-of-range setpoints. */
export function validateDeviceProfile(value: unknown): string[] {
  const errors: string[] = []
  const root = objectAt(
    value, "profile",
    ["kind", "schema_version", "deployment_status", "profile_id", "hardware", "control"],
    errors,
  )
  if (!root) return errors
  if (root.kind !== "bloomml.device-profile" || root.schema_version !== 1 ||
      root.deployment_status !== "draft_not_applied") {
    errors.push("profile: nieznana wersja/rodzaj lub próba zmiany statusu wdrożenia")
  }
  if (typeof root.profile_id !== "string" ||
      !/^[a-z0-9][a-z0-9-]{1,39}$/.test(root.profile_id)) {
    errors.push("profile_id: 2–40 małych liter, cyfr i myślników")
  }

  const hardware = objectAt(root.hardware, "hardware", ["board", "sensors", "outputs"], errors)
  if (hardware) {
    if (hardware.board !== "crowpanel-esp32s3-2_9-n8r8") {
      errors.push("hardware.board: nieobsługiwany profil sprzętowy")
    }
    const sensors = objectAt(hardware.sensors, "hardware.sensors", SENSOR_DEFINITIONS.map((x) => x.id), errors)
    if (sensors) {
      for (const item of SENSOR_DEFINITIONS) {
        const sensor = objectAt(sensors[item.id], "hardware.sensors." + item.id, ["installed"], errors)
        if (sensor && typeof sensor.installed !== "boolean") {
          errors.push(item.id + ".installed: wymagana wartość boolean")
        }
      }
    }
    const outputs = objectAt(hardware.outputs, "hardware.outputs", OUTPUT_DEFINITIONS.map((x) => x.id), errors)
    if (outputs) {
      for (const item of OUTPUT_DEFINITIONS) {
        const output = objectAt(outputs[item.id], "hardware.outputs." + item.id, ["installed", "endpoint"], errors)
        if (output) {
          if (typeof output.installed !== "boolean") errors.push(item.id + ".installed: wymagana wartość boolean")
          if (output.endpoint !== item.endpoint) {
            errors.push(item.id + ": endpoint niezgodny z obecnym hardware (wymagany " + item.endpoint + ")")
          }
        }
      }
    }
  }

  const control = objectAt(root.control, "control", ["humidity_mode", "sensor_timeout_ms", "schedule", "targets"], errors)
  if (control) {
    if (control.humidity_mode !== "RH" && control.humidity_mode !== "VPD") {
      errors.push("control.humidity_mode: RH albo VPD")
    }
    numberAt(control.sensor_timeout_ms, "control.sensor_timeout_ms", 5000, 300000, errors, true)
    const schedule = objectAt(control.schedule, "control.schedule", ["day_start_hour", "night_start_hour"], errors)
    if (schedule) {
      numberAt(schedule.day_start_hour, "day_start_hour", 0, 23, errors, true)
      numberAt(schedule.night_start_hour, "night_start_hour", 0, 23, errors, true)
      if (schedule.day_start_hour === schedule.night_start_hour) {
        errors.push("schedule: początek dnia i nocy musi być różny")
      }
    }
    const targets = objectAt(control.targets, "control.targets", ["day", "night"], errors)
    if (targets) {
      for (const period of ["day", "night"] as const) {
        const t = objectAt(targets[period], "targets." + period,
          ["air_temperature_c", "relative_humidity_pct", "air_vpd_kpa"], errors)
        if (t) {
          numberAt(t.air_temperature_c, period + ".temperature", 10, 40, errors)
          numberAt(t.relative_humidity_pct, period + ".rh", 20, 90, errors)
          numberAt(t.air_vpd_kpa, period + ".vpd", 0.2, 3.5, errors)
        }
      }
    }
  }
  return errors
}

/** Readiness is informative; even a complete draft cannot be applied by the current firmware. */
export function assessDeviceProfile(profile: DeviceProfile): string[] {
  const warnings: string[] = []
  if (!profile.hardware.sensors.tp357_inside.installed) {
    warnings.push("Brak TP357: firmware nie ma obecnie alternatywnego źródła T/RH dla sterowania.")
  }
  if (!profile.hardware.sensors.ds3231_clock.installed) {
    warnings.push("Brak DS3231: nie ma wiarygodnej podstawy harmonogramu dzień/noc.")
  }
  if (!profile.hardware.sensors.scd41_inside.installed) {
    warnings.push("Bez SCD41 pomiar CO₂ będzie niedostępny (dozownik CO₂ nie jest podłączony).")
  }
  if (!profile.hardware.sensors.xiaomi_nearby.installed) {
    warnings.push("Bez Xiaomi BLE brak kontekstu temperatury i wilgotności otoczenia.")
  }
  if (!profile.hardware.outputs.exhaust_fan.installed && !profile.hardware.outputs.humidifier.installed) {
    warnings.push("Brak urządzeń do regulacji klimatu: sam wybór nastaw nie zapewni ich osiągnięcia.")
  }
  return warnings
}

export type DeviceProfileParseResult =
  | { success: true; profile: DeviceProfile }
  | { success: false; errors: string[] }

export function parseDeviceProfileJson(raw: string): DeviceProfileParseResult {
  let value: unknown
  try {
    value = JSON.parse(raw) as unknown
  } catch {
    return { success: false, errors: ["Niepoprawny JSON"] }
  }
  const errors = validateDeviceProfile(value)
  if (errors.length) return { success: false, errors }
  return { success: true, profile: value as DeviceProfile }
}
