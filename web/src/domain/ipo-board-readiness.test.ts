import { describe, expect, it } from "vitest"
import { assessCrowPanelProfile, MAX_FIRMWARE_IPO_JSON_BYTES } from "./ipo-board-readiness"
import { assessIpoProfile, createDefaultIpoProfile, validateIpoProfile } from "./ipo-profile"

describe("DeviceProfile v2 / physical CrowPanel admission preview", () => {
  it("reports implemented NVS/readback without promising profile activation", () => {
    const warnings = assessIpoProfile(createDefaultIpoProfile()).join(" ")
    expect(warnings).toContain("NVS i readback v2")
    expect(warnings).toContain("aktywacja profilu jest zablokowana")
    expect(warnings).not.toContain("brak NVS/apply/readback")
  })

  it("admits only the compiled Mint hardware reference, without granting apply", () => {
    const p = createDefaultIpoProfile()
    expect(validateIpoProfile(p)).toEqual([])
    const status = assessCrowPanelProfile(p)
    expect(status.structurallyValid).toBe(true)
    expect(status.matchesCurrentFirmware).toBe(true)
    expect(status.applyEnabled).toBe(false)
    expect(status.reasons).toEqual([])
    expect(status.jsonBytes).toBeLessThanOrEqual(MAX_FIRMWARE_IPO_JSON_BYTES)
  })

  it("a valid generic IPO graph without a CO2 sensor is NOT board ready", () => {
    const p = createDefaultIpoProfile()
    p.signals = p.signals.filter((s) => s.role !== "inside.co2_ppm")
    expect(validateIpoProfile(p)).toEqual([])
    const status = assessCrowPanelProfile(p)
    expect(status.matchesCurrentFirmware).toBe(false)
    expect(status.reasons.join(" ")).toContain("inside.co2_ppm")
  })

  it("rejects physical apply if the safety-to-exhaust route is absent", () => {
    const p = createDefaultIpoProfile()
    p.links = p.links.filter((l) => l.process_id !== "co2_guard")
    expect(validateIpoProfile(p)).toEqual([])
    const status = assessCrowPanelProfile(p)
    expect(status.matchesCurrentFirmware).toBe(false)
    expect(status.reasons.join(" ")).toContain("co2_safety")
  })

  it("supports future driver descriptions as drafts, not as CrowPanel hardware", () => {
    const p = createDefaultIpoProfile()
    p.devices.push({
      id: "soil_1", label: "Gleba", driver: "adc.soil",
      locator: "adc:0", enabled: false,
    })
    expect(validateIpoProfile(p)).toEqual([])
    const status = assessCrowPanelProfile(p)
    expect(status.matchesCurrentFirmware).toBe(false)
    expect(status.reasons.some((r) => r.includes("soil_1"))).toBe(true)
  })

  it("rejects SCD41 temperature fallback until firmware support is qualified", () => {
    const p = createDefaultIpoProfile()
    const t = p.signals.find((s) => s.role === "inside.temperature_c")
    if (!t) throw new Error("missing sample temperature role")
    t.sources.push({ device_id: "scd41", channel: "temperature_c" })
    expect(validateIpoProfile(p)).toEqual([])
    const status = assessCrowPanelProfile(p)
    expect(status.matchesCurrentFirmware).toBe(false)
    expect(status.reasons.join(" ")).toContain("inside.temperature_c")
  })

  it("requires physical RF dwell budgets even for otherwise valid graphs", () => {
    const p = createDefaultIpoProfile()
    const fan = p.actuators.find((a) => a.role === "exhaust_fan")
    if (!fan) throw new Error("missing fan")
    fan.min_off_ms = 0
    expect(validateIpoProfile(p)).toEqual([])
    expect(assessCrowPanelProfile(p).reasons.join(" ")).toContain("ON/OFF")
  })

  it("never confuses an arbitrary valid IPO output role with actual RF hardware", () => {
    const p = createDefaultIpoProfile()
    p.actuators.push({
      id: "heater_1", device_id: "fan_socket", channel: "switch",
      role: "heater", safe_state: "off", min_on_ms: 120000, min_off_ms: 120000,
    })
    expect(validateIpoProfile(p).length).toBeGreaterThan(0) // duplicate physical channel
    const status = assessCrowPanelProfile(p)
    expect(status.structurallyValid).toBe(false)
    expect(status.applyEnabled).toBe(false)
  })

  it("detects a UTF-8 label accepted as Unicode text but too long for C++ bytes", () => {
    const p = createDefaultIpoProfile()
    p.devices[0].label = "ż".repeat(41) // 41 Unicode chars, 82 bytes
    expect(validateIpoProfile(p)).toEqual([])
    const status = assessCrowPanelProfile(p)
    expect(status.matchesCurrentFirmware).toBe(false)
    expect(status.reasons.join(" ")).toContain("80 bajtów")
  })

  it("the parser size budget is defined in UTF-8 bytes, not JS UTF-16 chars", () => {
    const p = createDefaultIpoProfile()
    p.devices[0].label = "Wewnątrz — mięta"
    const status = assessCrowPanelProfile(p)
    expect(status.jsonBytes).toBe(new TextEncoder().encode(JSON.stringify(p)).length)
    expect(status.applyEnabled).toBe(false)
  })
})
