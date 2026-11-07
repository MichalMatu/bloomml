import { useMemo, useRef, useState } from "react"
import {
  AppActionRow, AppErrorList, AppFormField, AppPage, AppPageFooter,
  AppPageHeader, AppSelectTrigger,
} from "@/components/app-chrome"
import { Badge } from "@/components/ui/badge"
import { Button } from "@/components/ui/button"
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card"
import { Input } from "@/components/ui/input"
import { Select, SelectContent, SelectItem, SelectValue } from "@/components/ui/select"
import { Switch } from "@/components/ui/switch"
import {
  DRIVER_CATALOG, EFFECT_LABELS, EFFECT_ROLE, OUTPUT_ROLES,
  PROCESS_CATALOG, PROCESS_KIND_IDS, SIGNAL_ROLES, SIGNAL_ROLE_IDS,
} from "@/domain/ipo-catalog"
import type { DriverId, OutputRole, ProcessKind, SignalRole } from "@/domain/ipo-catalog"
import {
  assessIpoProfile, createDefaultIpoProfile, createIpoDevice, DRIVER_IDS, inputChannels,
  outputChannels, parseIpoJson, validateIpoProfile,
} from "@/domain/ipo-profile"
import type { Actuator, Device, IpoProfileV2, Process, ProcessLink, Targets } from "@/domain/ipo-profile"
import { assessCrowPanelProfile } from "@/domain/ipo-board-readiness"
import { navigate, ROUTES } from "@/lib/routing"

function Pick({
  id, label, value, options, onChange,
}: {
  id: string
  label: string
  value: string
  options: readonly { value: string; label: string }[]
  onChange: (value: string) => void
}) {
  return (
    <AppFormField label={label} htmlFor={id}>
      <Select value={value || undefined} onValueChange={onChange}>
        <AppSelectTrigger id={id}>
          <SelectValue placeholder="Wybierz" />
        </AppSelectTrigger>
        <SelectContent>
          {options.map((o) => (
            <SelectItem key={o.value} value={o.value}>{o.label}</SelectItem>
          ))}
        </SelectContent>
      </Select>
    </AppFormField>
  )
}

const option = (value: string, label: string) => ({ value, label })
const idFromCount = (prefix: string, ids: readonly string[]): string => {
  let n = 1
  while (ids.includes(prefix + n)) n += 1
  return prefix + n
}
const sourceValue = (device_id: string, channel: string) => device_id + "|" + channel
function parseSource(value: string): { device_id: string; channel: string } | null {
  if (value === "none") return null
  const [device_id, channel] = value.split("|")
  return device_id && channel ? { device_id, channel } : null
}

export function IpoSetupPage() {
  const [profile, setProfile] = useState<IpoProfileV2>(createDefaultIpoProfile)
  const [newDriver, setNewDriver] = useState<DriverId>("rf433.binary_socket")
  const [newOutputRole, setNewOutputRole] = useState<OutputRole>("exhaust_fan")
  const [newProcessKind, setNewProcessKind] = useState<ProcessKind>("circulation")
  const [newLinkProcess, setNewLinkProcess] = useState("temperature")
  const [newLinkOutput, setNewLinkOutput] = useState("exhaust")
  const [message, setMessage] = useState<string | null>(null)
  const [importErrors, setImportErrors] = useState<string[]>([])
  const inputFile = useRef<HTMLInputElement>(null)
  const errors = useMemo(() => validateIpoProfile(profile), [profile])
  const warnings = useMemo(() => assessIpoProfile(profile), [profile])
  const valid = errors.length === 0
  const firmwareReadiness = useMemo(() => assessCrowPanelProfile(profile), [profile])

  const inputDevices = profile.devices.filter((d) =>
    DRIVER_CATALOG[d.driver].channels.some((c) => c.direction === "input"),
  )
  const outputDevices = profile.devices.filter((d) =>
    DRIVER_CATALOG[d.driver].channels.some((c) => c.direction === "output"),
  )
  const canAddDevice = profile.devices.length < 24
  const canAddActuator = profile.actuators.length < 24
  const canAddProcess = profile.processes.length < 16
  const canAddLink = profile.links.length < 48

  function changeDevice(id: string, patch: Partial<Device>) {
    setProfile((previous) => ({
      ...previous,
      devices: previous.devices.map((d) => d.id === id ? { ...d, ...patch } : d),
    }))
    setMessage(null)
  }
  function changeActuator(id: string, patch: Partial<Actuator>) {
    setProfile((previous) => ({
      ...previous,
      actuators: previous.actuators.map((a) => a.id === id ? { ...a, ...patch } : a),
    }))
    setMessage(null)
  }
  function changeProcess(id: string, patch: Partial<Process>) {
    setProfile((previous) => ({
      ...previous,
      processes: previous.processes.map((p) => p.id === id ? { ...p, ...patch } : p),
    }))
    setMessage(null)
  }
  function addDevice() {
    if (!canAddDevice) return
    const id = idFromCount("device_", profile.devices.map((d) => d.id))
    const created = createIpoDevice(id, newDriver)
    setProfile((previous) => ({ ...previous, devices: [...previous.devices, created] }))
    setMessage("Dodano urządzenie. Sprawdź adres; nowe urządzenia wymagają implementacji adaptera firmware.")
  }
  function removeDevice(id: string) {
    const used = profile.signals.some((s) => s.sources.some((source) => source.device_id === id)) ||
      profile.actuators.some((a) => a.device_id === id)
    if (used && !window.confirm("Usunąć urządzenie oraz powiązania sygnałów, wyjść i procesów, które z niego korzystają?")) return
    setProfile((previous) => {
      const detached = previous.actuators.filter((a) => a.device_id === id).map((a) => a.id)
      return {
        ...previous,
        devices: previous.devices.filter((d) => d.id !== id),
        signals: previous.signals.map((s) => ({
          ...s, sources: s.sources.filter((source) => source.device_id !== id),
        })),
        actuators: previous.actuators.filter((a) => a.device_id !== id),
        links: previous.links.filter((link) => !detached.includes(link.actuator_id)),
      }
    })
    setMessage("Usunięto urządzenie i zależne powiązania wyłącznie ze szkicu.")
  }
  function updateSource(role: SignalRole, priority: number, selected: string) {
    setProfile((previous) => {
      const binding = previous.signals.find((s) => s.role === role) ?? { role, sources: [] }
      const updated = [...binding.sources]
      const item = parseSource(selected)
      if (item) updated[priority] = item
      else updated.splice(priority, 1)
      const sources = updated.filter((s) => s != null)
      const signals = previous.signals.filter((s) => s.role !== role)
      return { ...previous, signals: [...signals, { role, sources }] }
    })
    setMessage(null)
  }
  function addActuator() {
    if (!canAddActuator || outputDevices.length === 0) return
    const used = new Set(profile.actuators.map((a) => a.device_id + "|" + a.channel))
    const device = outputDevices.find((d) =>
      outputChannels(d.driver).some((channel) => !used.has(d.id + "|" + channel)),
    )
    if (!device) { setMessage("Nie ma wolnego kanału wyjściowego. Dodaj urządzenie wykonawcze."); return }
    const channel = outputChannels(device.driver).find((c) => !used.has(device.id + "|" + c))
    if (!channel) return
    const item: Actuator = {
      id: idFromCount("output_", profile.actuators.map((a) => a.id)),
      device_id: device.id, channel, role: newOutputRole,
      safe_state: "off", min_on_ms: 0, min_off_ms: 0,
    }
    setProfile((previous) => ({ ...previous, actuators: [...previous.actuators, item] }))
    setMessage("Dodano logiczne wyjście. Wybierz rolę i czas minimum; nie zmienia to fizycznych wyjść.")
  }
  function removeActuator(id: string) {
    setProfile((previous) => ({
      ...previous,
      actuators: previous.actuators.filter((a) => a.id !== id),
      links: previous.links.filter((link) => link.actuator_id !== id),
    }))
    setMessage("Usunięto logiczne wyjście wraz z jego powiązaniami.")
  }
  function addProcess() {
    if (!canAddProcess) return
    if (newProcessKind === "co2_safety" && profile.processes.some((p) => p.kind === "co2_safety")) return
    const created: Process = {
      id: idFromCount("process_", profile.processes.map((p) => p.id)),
      kind: newProcessKind, enabled: true,
    }
    setProfile((previous) => ({ ...previous, processes: [...previous.processes, created] }))
    setMessage("Dodano proces do szkicu IPO. Obsługa nowych procesów w firmware wymaga implementacji.")
  }
  function removeProcess(id: string) {
    const process = profile.processes.find((p) => p.id === id)
    if (!process || PROCESS_CATALOG[process.kind].protected) return
    setProfile((previous) => ({
      ...previous, processes: previous.processes.filter((p) => p.id !== id),
      links: previous.links.filter((link) => link.process_id !== id),
    }))
    setMessage("Usunięto proces i jego połączenia.")
  }
  const linkProcess = profile.processes.find((p) => p.id === newLinkProcess)
  const linkActuator = profile.actuators.find((a) => a.id === newLinkOutput)
  const candidateEffect = linkProcess && linkActuator
    ? PROCESS_CATALOG[linkProcess.kind].effects.find((effect) => EFFECT_ROLE[effect] === linkActuator.role)
    : undefined
  function addLink() {
    if (!candidateEffect || !canAddLink || !linkProcess || !linkActuator) return
    const created: ProcessLink = {
      process_id: linkProcess.id, actuator_id: linkActuator.id, effect: candidateEffect,
    }
    if (profile.links.some((l) => l.process_id === created.process_id &&
      l.actuator_id === created.actuator_id && l.effect === created.effect)) {
      setMessage("To połączenie już istnieje.")
      return
    }
    setProfile((previous) => ({ ...previous, links: [...previous.links, created] }))
    setMessage("Dodano powiązanie. Ostateczne żądania muszą przechodzić przez OutputSupervisor.")
  }
  function updateControl(patch: Partial<IpoProfileV2["control"]>) {
    setProfile((previous) => ({ ...previous, control: { ...previous.control, ...patch } }))
    setMessage(null)
  }
  function updateTarget(period: "day" | "night", field: keyof Targets, value: number) {
    setProfile((previous) => ({
      ...previous, control: {
        ...previous.control,
        targets: { ...previous.control.targets, [period]: {
          ...previous.control.targets[period], [field]: value,
        } },
      },
    }))
    setMessage(null)
  }
  async function importFile(file: File) {
    const imported = parseIpoJson(await file.text())
    if (!imported.success) {
      setImportErrors(imported.errors)
      setMessage("Import odrzucony. Obsługiwane są wyłącznie profile v2 i jawna migracja v1.")
      return
    }
    setProfile(imported.profile)
    setImportErrors([])
    setMessage(imported.migrated
      ? "Zaimportowano v1 i jawnie przekonwertowano na szkic IPO v2. Zweryfikuj nowe powiązania."
      : "Zaimportowano szkic IPO v2. Nie wysłano go do kontrolera.")
  }
  function downloadDraft() {
    if (!valid) { setMessage("Najpierw popraw błędy konfiguracji."); return }
    // Compact JSON is the form bounded by the ESP32 firmware parser, unlike
    // pretty-printed drafts whose whitespace can exceed the device limit.
    const blob = new Blob([JSON.stringify(profile) + "\n"], { type: "application/json" })
    const url = URL.createObjectURL(blob)
    const a = document.createElement("a")
    a.href = url
    a.download = profile.profile_id + ".ipo-v2.draft.json"
    a.click()
    URL.revokeObjectURL(url)
    setMessage("Zapisano szkic IPO v2 na komputer. Urządzenie ESP32-S3 pozostało bez zmian.")
  }

  return (
    <AppPage width="wide">
      <AppPageHeader
        title="BloomML · mapa IPO"
        badges={<><Badge variant="secondary">Profil v2</Badge><Badge variant="outline">Szkic offline · bez apply</Badge></>}
        description="Jeden proces: urządzenia → sygnały → procesy → wyjścia → cele. Fizyczne urządzenie pozostaje sterowane przez sprawdzony Rule, Safety i OutputSupervisor."
        actions={<AppActionRow>
          <Button type="button" variant="outline" onClick={() => navigate(ROUTES.chamber3d)}>Wizualizacja 3D</Button>
          <Button type="button" variant="ghost" onClick={() => navigate(ROUTES.profileV1)}>Archiwalny profil v1</Button>
          <Button type="button" variant="ghost" onClick={() => navigate(ROUTES.legacyV5)}>Laboratorium v5</Button>
        </AppActionRow>}
      />

      <Card>
        <CardHeader>
          <CardTitle>1. Urządzenia — INPUT i OUTPUT</CardTitle>
          <CardDescription>
            Dowolne instancje z katalogu driverów, każde z własnym ID, adresem i
            kanałami. „Driver obecny” nie oznacza obsługi dynamicznego profilu w ESP32.
          </CardDescription>
        </CardHeader>
        <CardContent className="space-y-4">
          <div className="grid gap-3 lg:grid-cols-2">
            {profile.devices.map((d) => {
              const driver = DRIVER_CATALOG[d.driver]
              const mode = driver.channels.some((c) => c.direction === "input") ? "INPUT" : "OUTPUT"
              return (
                <div key={d.id} className="space-y-3 rounded-lg border p-3">
                  <div className="flex items-center justify-between gap-2">
                    <div className="flex items-center gap-2">
                      <Badge variant="outline">{mode}</Badge>
                      <span className="text-xs text-muted-foreground">{d.id}</span>
                    </div>
                    <div className="flex items-center gap-2">
                      <Switch aria-label={"Aktywne: " + d.id} checked={d.enabled}
                        onCheckedChange={(checked) => changeDevice(d.id, { enabled: checked })} />
                      <Button type="button" variant="ghost" size="sm"
                        onClick={() => removeDevice(d.id)}>Usuń</Button>
                    </div>
                  </div>
                  <p className="text-sm font-medium">{driver.label} · {driver.status === "proposed" ? "driver planowany" : "driver w firmware"}</p>
                  <div className="grid gap-3 sm:grid-cols-2">
                    <AppFormField label="Nazwa" htmlFor={d.id + "_label"}>
                      <Input id={d.id + "_label"} value={d.label}
                        onChange={(e) => changeDevice(d.id, { label: e.target.value })}/>
                    </AppFormField>
                    <AppFormField label="Adres / identyfikator" htmlFor={d.id + "_locator"}>
                      <Input id={d.id + "_locator"} value={d.locator}
                        onChange={(e) => changeDevice(d.id, { locator: e.target.value })}/>
                    </AppFormField>
                  </div>
                  <p className="text-xs text-muted-foreground">
                    Kanały: {driver.channels.map((c) => c.id + " [" + c.unit + "]").join(", ")}
                  </p>
                </div>
              )
            })}
          </div>
          <div className="flex flex-wrap items-end gap-3">
            <div className="min-w-64 flex-1">
              <Pick id="add_driver" label="Nowe urządzenie · typ drivera" value={newDriver}
                onChange={(v) => setNewDriver(v as DriverId)}
                options={DRIVER_IDS.map((id) => option(id, DRIVER_CATALOG[id].label +
                  (DRIVER_CATALOG[id].status === "proposed" ? " · planowany" : "")))}/>
            </div>
            <Button type="button" disabled={!canAddDevice} onClick={addDevice}>Dodaj urządzenie</Button>
          </div>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>2. Sygnały wejściowe — przypisywanie i fallback</CardTitle>
          <CardDescription>
            Jedna rola może korzystać z kilku czujników. Kolejność: główny →
            rezerwowy. Zgodność jednostek i kanałów jest obowiązkowa.
            SCD41 T/RH jest dopiero planowanym fallbackiem w firmware.
          </CardDescription>
        </CardHeader>
        <CardContent className="grid gap-3 lg:grid-cols-2">
          {SIGNAL_ROLE_IDS.map((role) => {
            const binding = profile.signals.find((s) => s.role === role)
            const possible = inputDevices.flatMap((device) =>
              inputChannels(device.driver, role).map((channel) => option(
                sourceValue(device.id, channel), device.label + " · " + channel,
              )),
            )
            const choices = [option("none", "Brak źródła"), ...possible]
            return (
              <div key={role} className="space-y-3 rounded-lg border p-3">
                <p className="text-sm font-medium">{SIGNAL_ROLES[role].label}</p>
                <p className="text-xs text-muted-foreground">Rola: {role} · {SIGNAL_ROLES[role].unit}</p>
                <div className="grid gap-3 sm:grid-cols-2">
                  <Pick id={role + "_primary"} label="Główny"
                    value={binding?.sources[0]
                      ? sourceValue(binding.sources[0].device_id, binding.sources[0].channel) : "none"}
                    options={choices} onChange={(value) => updateSource(role, 0, value)}/>
                  <Pick id={role + "_backup"} label="Rezerwowy"
                    value={binding?.sources[1]
                      ? sourceValue(binding.sources[1].device_id, binding.sources[1].channel) : "none"}
                    options={choices} onChange={(value) => updateSource(role, 1, value)}/>
                </div>
              </div>
            )
          })}
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>3. Wyjścia logiczne i urządzenia wykonawcze</CardTitle>
          <CardDescription>
            Oddzielamy rolę (np. wyciąg) od fizycznego endpointu (np. RF433 socket:1).
            Można dodać kolejne kanały i funkcje, ale runtime w tej chwili obsługuje
            tylko stałą mapę trzech gniazd. Stan bezpieczny wyjścia to OFF.
          </CardDescription>
        </CardHeader>
        <CardContent className="space-y-4">
          <div className="grid gap-3 lg:grid-cols-2">
            {profile.actuators.map((a) => (
              <div key={a.id} className="space-y-3 rounded-lg border p-3">
                <div className="flex items-center justify-between">
                  <p className="text-sm font-semibold">{a.id}</p>
                  <Button type="button" variant="ghost" size="sm"
                    onClick={() => removeActuator(a.id)}>Usuń</Button>
                </div>
                <div className="grid gap-3 sm:grid-cols-2">
                  <Pick id={a.id + "_role"} label="Funkcja" value={a.role}
                    options={Object.entries(OUTPUT_ROLES).map(([value,label]) => option(value,label))}
                    onChange={(v) => changeActuator(a.id, { role: v as OutputRole })}/>
                  <Pick id={a.id + "_device"} label="Urządzenie fizyczne"
                    value={a.device_id} options={outputDevices.map((d) => option(d.id, d.label + " · " + d.locator))}
                    onChange={(v) => {
                      const device = outputDevices.find((d) => d.id === v)
                      const channel = device ? outputChannels(device.driver)[0] : undefined
                      if (channel) changeActuator(a.id, { device_id: v, channel })
                    }}/>
                  <AppFormField label="Minimum ON (ms)" htmlFor={a.id + "_on"}>
                    <Input id={a.id + "_on"} type="number" min={0} max={3600000} step={1000}
                      value={a.min_on_ms}
                      onChange={(e) => changeActuator(a.id, { min_on_ms: Number(e.target.value) })}/>
                  </AppFormField>
                  <AppFormField label="Minimum OFF (ms)" htmlFor={a.id + "_off"}>
                    <Input id={a.id + "_off"} type="number" min={0} max={3600000} step={1000}
                      value={a.min_off_ms}
                      onChange={(e) => changeActuator(a.id, { min_off_ms: Number(e.target.value) })}/>
                  </AppFormField>
                </div>
                <p className="text-xs text-muted-foreground">Kanał: {a.channel} · fail-safe OFF · bez bezpośrednich komend GPIO/RF</p>
              </div>
            ))}
          </div>
          <div className="flex flex-wrap items-end gap-3">
            <div className="min-w-48 flex-1">
              <Pick id="new_output_role" label="Funkcja nowego wyjścia" value={newOutputRole}
                options={Object.entries(OUTPUT_ROLES).map(([value,label]) => option(value,label))}
                onChange={(value) => setNewOutputRole(value as OutputRole)}/>
            </div>
            <Button type="button" disabled={!canAddActuator || outputDevices.length === 0}
              onClick={addActuator}>Dodaj wyjście</Button>
          </div>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>4. Processing — powiązania proces → działanie → wyjście</CardTitle>
          <CardDescription>
            Jeden wentylator może realizować żądania od temperatury, wilgotności i ochrony CO₂.
            Dla zgodnych efektów propozycje łączy arbiter MAX, natomiast Safety i
            OutputSupervisor mają nadrzędność. ML Shadow nie wysyła komend.
          </CardDescription>
        </CardHeader>
        <CardContent className="space-y-4">
          <div className="grid gap-2 lg:grid-cols-2">
            {profile.processes.map((p) => {
              const definition = PROCESS_CATALOG[p.kind]
              return (
                <div key={p.id} className="flex items-center justify-between gap-3 rounded-lg border p-3">
                  <div>
                    <p className="text-sm font-medium">{definition.label}</p>
                    <p className="text-xs text-muted-foreground">{p.id} · {definition.status} {definition.protected ? "· zabezpieczenie nieusuwalne" : ""}</p>
                  </div>
                  <div className="flex items-center gap-2">
                    <Switch aria-label={"Proces: " + p.id} checked={p.enabled}
                      disabled={definition.protected}
                      onCheckedChange={(enabled) => changeProcess(p.id, { enabled })}/>
                    {!definition.protected ? (
                      <Button type="button" variant="ghost" size="sm"
                        onClick={() => removeProcess(p.id)}>Usuń</Button>
                    ) : null}
                  </div>
                </div>
              )
            })}
          </div>
          <div className="flex flex-wrap items-end gap-3">
            <div className="min-w-48 flex-1">
              <Pick id="new_process_kind" label="Nowy typ procesu" value={newProcessKind}
                options={PROCESS_KIND_IDS.map((id) => option(id, PROCESS_CATALOG[id].label +
                  (PROCESS_CATALOG[id].status === "proposed" ? " · planowany" : "")))}
                onChange={(value) => setNewProcessKind(value as ProcessKind)}/>
            </div>
            <Button type="button" disabled={!canAddProcess}
              onClick={addProcess}>Dodaj proces</Button>
          </div>
          <div className="space-y-2">
            <p className="font-semibold">Mapa powiązań</p>
            {profile.links.map((link, index) => (
              <div key={index} className="flex items-center justify-between gap-3 rounded-lg border px-3 py-2">
                <p className="text-sm">
                  <strong>{profile.processes.find((p) => p.id === link.process_id)?.id ?? link.process_id}</strong>
                  {" → "}{EFFECT_LABELS[link.effect]}{" → "}
                  <strong>{profile.actuators.find((a) => a.id === link.actuator_id)?.id ?? link.actuator_id}</strong>
                </p>
                <Button type="button" variant="ghost" size="sm"
                  onClick={() => setProfile((previous) => ({
                    ...previous, links: previous.links.filter((_, i) => i !== index),
                  }))}>Usuń</Button>
              </div>
            ))}
          </div>
          <div className="grid items-end gap-3 lg:grid-cols-[1fr_1fr_auto]">
            <Pick id="link_process" label="Proces" value={newLinkProcess}
              options={profile.processes.map((p) => option(p.id, PROCESS_CATALOG[p.kind].label))}
              onChange={setNewLinkProcess}/>
            <Pick id="link_output" label="Wyjście" value={newLinkOutput}
              options={profile.actuators.map((a) => option(a.id, a.id + " · " + OUTPUT_ROLES[a.role]))}
              onChange={setNewLinkOutput}/>
            <Button type="button" disabled={!candidateEffect || !canAddLink}
              onClick={addLink}>
              {candidateEffect ? "Połącz: " + EFFECT_LABELS[candidateEffect] : "Niekompatybilne"}
            </Button>
          </div>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>5. Cele środowiskowe i harmonogram</CardTitle>
          <CardDescription>
            Nastawy procesu, nie komendy dla przekaźników. VPD liczymy z T/RH.
            CO₂ jest obecnie monitorowany, bez aktywnego dozowania; zabezpieczenia
            termiczne lampy są nieedytowalne.
          </CardDescription>
        </CardHeader>
        <CardContent className="space-y-4">
          <div className="grid gap-3 md:grid-cols-4">
            <AppFormField label="Profil ID" htmlFor="ipo_profile_id">
              <Input id="ipo_profile_id" value={profile.profile_id} onChange={(e) =>
                setProfile((p) => ({ ...p, profile_id: e.target.value }))}/>
            </AppFormField>
            <Pick id="humidity_mode_v2" label="Tryb wilgotności" value={profile.control.humidity_mode}
              options={[option("RH", "RH (%)"), option("VPD", "VPD (kPa)")]}
              onChange={(mode) => updateControl({ humidity_mode: mode as "RH" | "VPD" })}/>
            <AppFormField label="Początek dnia (godz.)" htmlFor="ipo_day">
              <Input id="ipo_day" type="number" min={0} max={23} step={1}
                value={profile.control.schedule.day_start_hour}
                onChange={(e) => updateControl({ schedule: {
                  ...profile.control.schedule, day_start_hour: Number(e.target.value),
                } })}/>
            </AppFormField>
            <AppFormField label="Początek nocy (godz.)" htmlFor="ipo_night">
              <Input id="ipo_night" type="number" min={0} max={23} step={1}
                value={profile.control.schedule.night_start_hour}
                onChange={(e) => updateControl({ schedule: {
                  ...profile.control.schedule, night_start_hour: Number(e.target.value),
                } })}/>
            </AppFormField>
          </div>
          <div className="grid gap-3 md:grid-cols-2">
            {(["day", "night"] as const).map((period) => (
              <div key={period} className="grid gap-3 rounded-lg border p-3">
                <p className="font-semibold">{period === "day" ? "Dzień" : "Noc"}</p>
                <AppFormField label="Temperatura (°C)" htmlFor={"ipo_t_" + period}>
                  <Input id={"ipo_t_" + period} type="number" min={10} max={40} step={0.5}
                    value={profile.control.targets[period].air_temperature_c}
                    onChange={(e) => updateTarget(period, "air_temperature_c", Number(e.target.value))}/>
                </AppFormField>
                <AppFormField label={profile.control.humidity_mode === "RH" ? "RH (%)" : "VPD (kPa)"}
                  htmlFor={"ipo_h_" + period}>
                  <Input id={"ipo_h_" + period} type="number"
                    min={profile.control.humidity_mode === "RH" ? 20 : 0.2}
                    max={profile.control.humidity_mode === "RH" ? 90 : 3.5}
                    step={profile.control.humidity_mode === "RH" ? 1 : 0.1}
                    value={profile.control.humidity_mode === "RH"
                      ? profile.control.targets[period].relative_humidity_pct
                      : profile.control.targets[period].air_vpd_kpa}
                    onChange={(e) => updateTarget(period,
                      profile.control.humidity_mode === "RH" ? "relative_humidity_pct" : "air_vpd_kpa",
                      Number(e.target.value))}/>
                </AppFormField>
              </div>
            ))}
          </div>
          <p className="text-xs text-muted-foreground">
            Czujniki mają flagi valid/age; brak lub stary odczyt nie oznacza zero.
            Obecne firmware nie realizuje jeszcze przełączania źródeł ani nastaw z profilu v2.
            Lamp safety: trip ≥28°C; recovery ≤26°C przez 10 min.
          </p>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>6. Walidacja grafu i zapis szkicu</CardTitle>
          <CardDescription>
            Eksport v2 i jawny import/migracja v1. Nie ma fizycznego apply ani NVS.
            Monitorowanie i readback aktywnego urządzenia zostaną podłączone dopiero
            po bezpiecznym adapterze runtime.
          </CardDescription>
        </CardHeader>
        <CardContent className="space-y-4">
          <div className="grid gap-3 md:grid-cols-4">
            <div className="rounded-lg border p-3"><p className="text-2xl font-semibold">{profile.devices.length}</p><p className="text-xs text-muted-foreground">Urządzenia</p></div>
            <div className="rounded-lg border p-3"><p className="text-2xl font-semibold">{profile.signals.length}</p><p className="text-xs text-muted-foreground">Role sygnałów</p></div>
            <div className="rounded-lg border p-3"><p className="text-2xl font-semibold">{profile.actuators.length}</p><p className="text-xs text-muted-foreground">Wyjścia</p></div>
            <div className="rounded-lg border p-3"><p className="text-2xl font-semibold">{profile.links.length}</p><p className="text-xs text-muted-foreground">Powiązania IPO</p></div>
          </div>
          {valid ? <p className="text-sm font-medium">Graf i dane szkicu: poprawne strukturalnie. To NIE jest kwalifikacja do sterowania sprzętem.</p>
            : <AppErrorList errors={errors}/>}
          <div className="space-y-2 rounded-lg border p-3">
            <p className="text-sm font-semibold">Zgodność z obecnym firmware CrowPanel</p>
            <p className="text-sm text-muted-foreground">
              {firmwareReadiness.matchesCurrentFirmware
                ? "Profil mieści się w aktualnych możliwościach referencyjnej mapy Mint."
                : "Profil jest poprawnym projektem IPO, ale obecne firmware nie obsługuje całej konfiguracji."}
              {" "}Rozmiar JSON: {firmwareReadiness.jsonBytes} bajtów.
              {" "}Aktywacja na urządzeniu: niedostępna; brak kwalifikowanego apply.
            </p>
            {!firmwareReadiness.matchesCurrentFirmware ? (
              <details>
                <summary className="cursor-pointer text-sm font-medium">
                  Przyczyny niezgodności ({firmwareReadiness.reasons.length})
                </summary>
                <ul className="mt-2 list-disc space-y-1 pl-5 text-sm text-muted-foreground">
                  {firmwareReadiness.reasons.map((reason) => <li key={reason}>{reason}</li>)}
                </ul>
              </details>
            ) : null}
          </div>
          <details className="rounded-lg border p-3">
            <summary className="cursor-pointer text-sm font-medium">Zastrzeżenia sprzętowe i gotowość</summary>
            <ul className="mt-2 list-disc space-y-1 pl-5 text-sm text-muted-foreground">
              {warnings.map((w) => <li key={w}>{w}</li>)}
            </ul>
          </details>
          <p className="text-sm text-muted-foreground">
            Profil NIE jest zastosowany. Brak danych z fizycznego urządzenia,
            brak potwierdzonych stanów RF433. Zabezpieczenia i wyjścia należą nadal
            wyłącznie do aktywnego firmware.
          </p>
          <AppActionRow>
            <Button type="button" disabled={!valid} onClick={downloadDraft}>Pobierz DeviceProfile v2</Button>
            <Button type="button" variant="outline" onClick={() => inputFile.current?.click()}>Wczytaj v2 lub migruj v1</Button>
            <Button type="button" variant="ghost" onClick={() => {
              setProfile(createDefaultIpoProfile())
              setImportErrors([])
              setMessage("Przywrócono przykładowy profil Mint IPO v2.")
            }}>Przywróć Mint</Button>
            <input ref={inputFile} type="file" hidden accept=".json,application/json"
              onChange={(e) => {
                const file = e.target.files?.[0]
                e.target.value = ""
                if (file) void importFile(file)
              }}/>
          </AppActionRow>
          {message ? <p className="text-sm text-muted-foreground">{message}</p> : null}
          <AppErrorList errors={importErrors}/>
        </CardContent>
      </Card>

      <AppPageFooter>
        Źródło: <code>schemas/device-profile.v2.schema.json</code> ·
        <code>docs/CONFIGURATION_FLOW.md</code>. ML Shadow wyłącznie obserwuje.
      </AppPageFooter>
    </AppPage>
  )
}
