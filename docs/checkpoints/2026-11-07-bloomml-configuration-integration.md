# BloomML — Configuration & Runtime Integration, part A (2026-11-07)

## Scope completed

Observed production code before changing the interface:
- `RuntimeInsideSource` takes T/RH from TP357 BLE and CO₂ only from SCD41; `RuntimeNearbySource` takes Xiaomi BLE T/RH, DS3231/clock selects local hour.
- `ScheduleProfile.cpp` hardcodes Mint daytime 06–22: 24.5°C / 58% RH / 1.2kPa; nighttime 21.5°C / 65% RH / 0.9kPa; 30-second sensor timeout, RH mode; CO₂ targets disabled.
- `OutputBindings.cpp` maps exhaust→socket 1, scheduled lamp→socket 2, humidifier→socket 3. Four climate-v6 conceptual ML roles have no configured physical outputs. Lamp thermal interlock and `OutputSupervisor` safety ownership remain unchanged.

Delivered operator draft:
- `schemas/device-profile.v1.schema.json` and `profiles/crowpanel-mint.v1.json`.
- `web/src/domain/device-profile.ts`: bounded strict shape/range/endpoint validator, profile completeness warnings, JSON import.
- `web/src/pages/setup-page.tsx`: one legible equipment / target / schedule / summary + export view.
- `web/src/app-router.tsx` and `web/src/lib/routing.ts`: root operator draft; old v5 research editor under `/lab/v5`; 3D still independent.
- `docs/CONFIGURATION_FLOW.md`: canonical one-place mapping with supported, unsupported and only-observable parameters.

## Truth and boundary

**Not completed:** ESP32 firmware profile import, active NVS profile save/recovery/rollback, applying settings, live status/readback, hardware sensor/output confirmation, e-ink data parity. The exported profile has forced `deployment_status=draft_not_applied`; it is not a firmware command. No physical writes, training or shadow-to-active ML promotion were performed.

**Verification:** Operator instructed **no tests or heavy builds**. None executed for this sprint. Code and documentation committed; validation status of the new frontend/runtime integration remains **UNVERIFIED**, not PASS. Previous historical tests are not counted as testing these changes.

## Next

Single backend milestone: safe firmware parse/validate/persist/apply/readback of the v1 profile using existing owners, still with fixed RF433 bindings and separate device/ML contracts. Detailed steps: `../NEXT_CHAT.md`; I/O matrix: `../CONFIGURATION_FLOW.md`.
