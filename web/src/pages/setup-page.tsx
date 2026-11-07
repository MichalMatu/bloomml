import { useMemo, useRef, useState } from "react"

import {
  AppActionRow,
  AppCardBody,
  AppErrorList,
  AppFormField,
  AppPage,
  AppPageFooter,
  AppPageHeader,
} from "@/components/app-chrome"
import { Badge } from "@/components/ui/badge"
import { Button } from "@/components/ui/button"
import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from "@/components/ui/card"
import { Input } from "@/components/ui/input"
import {
  Select,
  SelectContent,
  SelectItem,
  SelectValue,
} from "@/components/ui/select"
import { AppSelectTrigger } from "@/components/app-chrome"
import { Switch } from "@/components/ui/switch"
import {
  assessDeviceProfile,
  createDefaultDeviceProfile,
  OUTPUT_DEFINITIONS,
  parseDeviceProfileJson,
  SENSOR_DEFINITIONS,
  validateDeviceProfile,
} from "@/domain/device-profile"
import type {
  DeviceProfile,
  EnvironmentTargets,
  HumidityMode,
  OutputId,
  Period,
  SensorId,
} from "@/domain/device-profile"
import { navigate, ROUTES } from "@/lib/routing"

const periods: { id: Period; label: string }[] = [
  { id: "day", label: "Dzień" },
  { id: "night", label: "Noc" },
]

export function SetupPage() {
  const [profile, setProfile] = useState<DeviceProfile>(createDefaultDeviceProfile)
  const [importErrors, setImportErrors] = useState<string[]>([])
  const [message, setMessage] = useState<string | null>(null)
  const fileRef = useRef<HTMLInputElement>(null)

  const validationErrors = useMemo(() => validateDeviceProfile(profile), [profile])
  const warnings = useMemo(() => assessDeviceProfile(profile), [profile])
  const valid = validationErrors.length === 0

  function updateSensor(id: SensorId, installed: boolean) {
    setProfile((current) => ({
      ...current,
      hardware: {
        ...current.hardware,
        sensors: {
          ...current.hardware.sensors,
          [id]: { installed },
        },
      },
    }))
    setMessage(null)
  }

  function updateOutput(id: OutputId, installed: boolean) {
    setProfile((current) => ({
      ...current,
      hardware: {
        ...current.hardware,
        outputs: {
          ...current.hardware.outputs,
          [id]: { ...current.hardware.outputs[id], installed },
        },
      },
    }))
    setMessage(null)
  }

  function updateTarget(period: Period, key: keyof EnvironmentTargets, value: number) {
    setProfile((current) => ({
      ...current,
      control: {
        ...current.control,
        targets: {
          ...current.control.targets,
          [period]: { ...current.control.targets[period], [key]: value },
        },
      },
    }))
    setMessage(null)
  }

  function updateSchedule(key: "day_start_hour" | "night_start_hour", value: number) {
    setProfile((current) => ({
      ...current,
      control: {
        ...current.control,
        schedule: { ...current.control.schedule, [key]: value },
      },
    }))
    setMessage(null)
  }

  function downloadDraft() {
    if (!valid) {
      setMessage("Popraw pola profilu przed eksportem.")
      return
    }
    const blob = new Blob([JSON.stringify(profile, null, 2) + "\n"], {
      type: "application/json",
    })
    const url = URL.createObjectURL(blob)
    const a = document.createElement("a")
    a.href = url
    a.download = profile.profile_id + ".device-profile.v1.draft.json"
    a.click()
    URL.revokeObjectURL(url)
    setMessage("Pobrano szkic. Nie został wysłany do ESP32-S3.")
  }

  async function importDraft(file: File) {
    const parsed = parseDeviceProfileJson(await file.text())
    if (!parsed.success) {
      setImportErrors(parsed.errors)
      setMessage("Import odrzucony. Użyj profilu device-profile v1, nie v4/v5/v6 ML.")
      return
    }
    setProfile(parsed.profile)
    setImportErrors([])
    setMessage("Wczytano szkic " + file.name + ". Nie został zastosowany na urządzeniu.")
  }

  const humidityFields =
    profile.control.humidity_mode === "RH"
      ? { key: "relative_humidity_pct" as const, label: "Wilgotność docelowa (%)", min: 20, max: 90, step: 1 }
      : { key: "air_vpd_kpa" as const, label: "Docelowy VPD (kPa)", min: 0.2, max: 3.5, step: 0.1 }

  return (
    <AppPage width="wide">
      <AppPageHeader
        title="BloomML · archiwalny szkic profilu v1"
        badges={
          <>
            <Badge variant="secondary">Profil v1 · szkic</Badge>
            <Badge variant="outline">Nie zastosowano na ESP32-S3</Badge>
          </>
        }
        description={
          <>
            Jeden przepływ: wyposażenie → cele → harmonogram → podsumowanie.
            Pokazujemy wyłącznie urządzenia zmapowane w aktualnym firmware CrowPanel.
            Zmiany w tej stronie nie przełączają przekaźników.
          </>
        }
        actions={
          <AppActionRow>
            <Button type="button" onClick={() => navigate(ROUTES.configurator)}>Nowy konfigurator IPO v2</Button>
            <Button type="button" variant="outline" onClick={() => navigate(ROUTES.chamber3d)}>
              Wizualizacja 3D
            </Button>
            <Button type="button" variant="ghost" onClick={() => navigate(ROUTES.legacyV5)}>
              Laboratorium v5 · 228 pól
            </Button>
          </AppActionRow>
        }
      />

      <Card>
        <CardHeader>
          <CardTitle>1. Czujniki · co naprawdę mierzymy</CardTitle>
          <CardDescription>
            Zainstalowany oznacza deklarację wyposażenia, nie wykrycie przez firmware.
            Źródła sygnałów są obecnie przypisane na stałe w runtime.
          </CardDescription>
        </CardHeader>
        <CardContent>
          <div className="grid gap-3 md:grid-cols-2">
            {SENSOR_DEFINITIONS.map((sensor) => (
              <div key={sensor.id} className="flex items-start justify-between gap-4 rounded-lg border p-4">
                <div className="min-w-0 space-y-1">
                  <p className="font-medium">{sensor.label}</p>
                  <p className="text-sm text-muted-foreground">{sensor.signals}</p>
                  <p className="text-xs text-muted-foreground">{sensor.role}</p>
                  {sensor.required ? <Badge variant="outline">Wymagany w obecnym runtime</Badge> : null}
                </div>
                <Switch
                  aria-label={"Zainstalowany: " + sensor.label}
                  checked={profile.hardware.sensors[sensor.id].installed}
                  onCheckedChange={(checked) => updateSensor(sensor.id, checked)}
                />
              </div>
            ))}
          </div>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>2. Wyjścia · co może zmieniać klimat</CardTitle>
          <CardDescription>
            Trzy aktualne kanały RF433, bez dowolnego przepinania ról. Nie myl możliwości
            kontraktu ML (6 ról) z trzema podłączonymi kanałami.
          </CardDescription>
        </CardHeader>
        <CardContent>
          <div className="grid gap-3 md:grid-cols-3">
            {OUTPUT_DEFINITIONS.map((output) => (
              <div key={output.id} className="space-y-2 rounded-lg border p-4">
                <div className="flex items-center justify-between gap-3">
                  <p className="font-medium">{output.label}</p>
                  <Switch
                    aria-label={"Zainstalowane wyjście: " + output.label}
                    checked={profile.hardware.outputs[output.id].installed}
                    onCheckedChange={(checked) => updateOutput(output.id, checked)}
                  />
                </div>
                <p className="text-xs font-mono text-muted-foreground">{output.endpoint}</p>
                <p className="text-sm text-muted-foreground">{output.description}</p>
                <Badge variant="outline">{output.kind === "schedule" ? "Harmonogram + safety" : "Rule / safety"}</Badge>
              </div>
            ))}
          </div>
          <p className="mt-3 text-sm text-muted-foreground">
            Grzałka, chłodzenie, osuszacz, dozownik CO₂ i wentylator obiegowy są
            rolami koncepcyjnymi lub niepodłączonymi w bieżącym profilu, a nie
            dostępnymi przyciskami tego urządzenia.
          </p>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>3. Cele środowiskowe i dzień/noc</CardTitle>
          <CardDescription>
            Temperatura i RH lub VPD są nastawami sterowania. CO₂ z SCD41 jest
            obecnie tylko odczytem: brak podłączonego dozownika CO₂.
            Druga nastawa wilgotności jest zachowywana w szkicu, ale nie jest aktywna.
          </CardDescription>
        </CardHeader>
        <CardContent>
          <AppCardBody variant="form">
            <div className="grid gap-4 md:grid-cols-3">
              <AppFormField label="ID profilu" htmlFor="profile_id">
                <Input
                  id="profile_id"
                  value={profile.profile_id}
                  onChange={(e) => {
                    setProfile((current) => ({ ...current, profile_id: e.target.value }))
                    setMessage(null)
                  }}
                />
              </AppFormField>
              <AppFormField label="Sterowanie wilgotnością" htmlFor="humidity_mode">
                <Select
                  value={profile.control.humidity_mode}
                  onValueChange={(value) => {
                    if (value !== "RH" && value !== "VPD") return
                    const mode: HumidityMode = value
                    setProfile((current) => ({
                      ...current,
                      control: { ...current.control, humidity_mode: mode },
                    }))
                    setMessage(null)
                  }}
                >
                  <AppSelectTrigger id="humidity_mode">
                    <SelectValue placeholder="Tryb" />
                  </AppSelectTrigger>
                  <SelectContent>
                    <SelectItem value="RH">Wilgotność względna (RH)</SelectItem>
                    <SelectItem value="VPD">Deficyt ciśnienia pary (VPD)</SelectItem>
                  </SelectContent>
                </Select>
              </AppFormField>
              <AppFormField label="Ważność pomiaru (ms)" htmlFor="sensor_timeout_ms">
                <Input
                  id="sensor_timeout_ms"
                  type="number"
                  min={5000}
                  max={300000}
                  step={1000}
                  value={profile.control.sensor_timeout_ms}
                  onChange={(e) =>
                    setProfile((current) => ({
                      ...current,
                      control: { ...current.control, sensor_timeout_ms: Number(e.target.value) },
                    }))
                  }
                />
              </AppFormField>
            </div>

            <div className="grid gap-4 md:grid-cols-2">
              <AppFormField label="Początek dnia (godzina lokalna)" htmlFor="day_start_hour">
                <Input
                  id="day_start_hour"
                  type="number"
                  min={0}
                  max={23}
                  step={1}
                  value={profile.control.schedule.day_start_hour}
                  onChange={(e) => updateSchedule("day_start_hour", Number(e.target.value))}
                />
              </AppFormField>
              <AppFormField label="Początek nocy (godzina lokalna)" htmlFor="night_start_hour">
                <Input
                  id="night_start_hour"
                  type="number"
                  min={0}
                  max={23}
                  step={1}
                  value={profile.control.schedule.night_start_hour}
                  onChange={(e) => updateSchedule("night_start_hour", Number(e.target.value))}
                />
              </AppFormField>
            </div>

            <div className="grid gap-4 md:grid-cols-2">
              {periods.map((period) => (
                <div key={period.id} className="space-y-4 rounded-lg border p-4">
                  <p className="font-semibold">{period.label}</p>
                  {([
                    { key: "air_temperature_c" as const, label: "Docelowa temperatura (°C)", min: 10, max: 40, step: 0.5 },
                    humidityFields,
                  ]).map((field) => (
                    <AppFormField
                      key={field.key}
                      label={field.label}
                      htmlFor={period.id + "_" + field.key}
                    >
                      <Input
                        id={period.id + "_" + field.key}
                        type="number"
                        min={field.min}
                        max={field.max}
                        step={field.step}
                        value={profile.control.targets[period.id][field.key]}
                        onChange={(e) => updateTarget(period.id, field.key, Number(e.target.value))}
                      />
                    </AppFormField>
                  ))}
                </div>
              ))}
            </div>
            <p className="text-sm text-muted-foreground">
              Światło: dzień = ON, noc = OFF. Zabezpieczenie temperatury lampy
              (wyłączenie od 28°C, powrót po ≤26°C przez 10 min) nie jest
              edytowalne z konfiguratora.
            </p>
          </AppCardBody>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>4. Przegląd profilu i zapis</CardTitle>
          <CardDescription>
            Ten etap tworzy zweryfikowany szkic, nie zapis NVS i nie wdrożenie.
            Fizyczny runtime ma nadal stały profil Mint. Zastosowanie ustawień,
            odczyt statusu ESP32-S3 i restart wymagają osobnego adaptera firmware.
          </CardDescription>
        </CardHeader>
        <CardContent>
          <AppCardBody variant="stack">
            <p className="text-sm">
              <strong>Ścieżka sterowania:</strong> pomiary → Rule (ML shadow bez prawa
              wykonywania) → arbitraż/safety → OutputSupervisor → RF433 → telemetria → e-ink.
            </p>
            <p className="text-sm">
              <strong>Założone kanały:</strong> TP357 (T/RH) · SCD41 (CO₂) · Xiaomi
              (otoczenie) · DS3231 (czas). Brak odczytu nie oznacza wartości zero.
            </p>
            <p className="text-sm">
              <strong>Status połączenia:</strong> brak odczytu urządzenia z tej strony.
              To formularz offline, nie monitoring na żywo.
            </p>
            <AppErrorList errors={validationErrors} />
            {warnings.length > 0 ? (
              <div className="rounded-lg border p-3">
                <p className="text-sm font-medium">Uwagi o wybranym wyposażeniu</p>
                <ul className="mt-2 list-disc space-y-1 pl-5 text-sm text-muted-foreground">
                  {warnings.map((item) => <li key={item}>{item}</li>)}
                </ul>
              </div>
            ) : null}
            <AppActionRow>
              <Button type="button" disabled={!valid} onClick={downloadDraft}>
                Pobierz profil v1 · szkic
              </Button>
              <Button type="button" variant="outline" onClick={() => fileRef.current?.click()}>
                Wczytaj szkic v1
              </Button>
              <Button type="button" variant="ghost" onClick={() => {
                setProfile(createDefaultDeviceProfile())
                setImportErrors([])
                setMessage("Przywrócono domyślny szkic Mint.")
              }}>
                Domyślny Mint
              </Button>
              <input
                ref={fileRef}
                hidden
                type="file"
                accept=".json,application/json"
                onChange={(event) => {
                  const file = event.target.files?.[0]
                  event.target.value = ""
                  if (file) void importDraft(file)
                }}
              />
            </AppActionRow>
            {message ? <p className="text-sm text-muted-foreground">{message}</p> : null}
            <AppErrorList errors={importErrors} />
          </AppCardBody>
        </CardContent>
      </Card>

      <AppPageFooter>
        Kontrakt: <code>schemas/device-profile.v1.schema.json</code> · matryca sterowania:
        <code> docs/CONFIGURATION_FLOW.md</code> · obecnie tryb draft-only.
      </AppPageFooter>
    </AppPage>
  )
}
