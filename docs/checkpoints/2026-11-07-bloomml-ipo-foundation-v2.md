# BloomML — IPO Foundation v2 sprint (2026-11-07)

## Implemented on main

- New versioned, **bounded and draft-only** contract: `schemas/device-profile.v2.schema.json`, preset `profiles/crowpanel-ipo.v2.json`.
- Extensible frontend capability/driver catalog: `web/src/domain/ipo-catalog.ts`. Includes current TP357/Xiaomi/SCD41/DS3231/RF433 and explicitly proposed BME280/BH1750/ADC/GPIO/PWM/Modbus drivers.
- Strict topology validation + input priority/units, deduped physical channels, process-effect compatibility, output safe-OFF and unremovable CO₂ safety declaration: `web/src/domain/ipo-profile.ts`.
- Explicit validated v1 → v2 import migration, no v4/v5 inference conversion. Previous v1/v5 pages remain accessible.
- New configurable single operator homepage `web/src/pages/ipo-setup-page.tsx` and routing: add/remove devices, choose primary/fallback sensor signal, define logical outputs, enable typed processes, create/remove multi-process-to-output links, change day/night T + RH/VPD, validate, import/export JSON.
- C++17 portable `lib/ipo_profile/src/IpoGraph.h`: static-capacity device/signal/output/process/link graph, structural compatibility validator, symbolic max mixer. **Not included in ESP-IDF production build, no actual hardware commands**.
- Single canonical reference: `docs/CONFIGURATION_FLOW.md`.

## Preserved

- Current firmware runs hardcoded Mint with real TP357 T/RH, SCD41 CO₂, Xiaomi ambient, DS3231 time and three fixed RF433 channels (fan, lamp, humidifier). SCD41 T/RH fallback and dynamic output mapping not yet implemented on board.
- Rule remains authoritative, ML shadow is passive; Safety and OutputSupervisor remain the only real execution path. No NVS/profile apply/readback, no RF433/ESP/display behavior changes.
- Legacy browser v1 and research v5 remain available, as does the separate 3D page.

## Not completed / explicit evidence

- **No tests, typecheck, lint, builds, physical flashing or deployment:** operator requested not to repeat existing tests or heavy compilations. New source is **UNVERIFIED** pending a separate targeted qualification.
- No physical v2 registry, driver discovery, v2 JSON parser, ESP32 profile NVS persistence, atomic apply, actual readback, live panel, telemetry mapping, simulated v2 driver adapters, or MPC promotion.
- v2 profiles may include *planned drivers*, but they cannot be promoted to hardware configuration without a firmware-side capability validator and explicit acceptance.
- Browser symbolic arbitration is NOT the final OutputSupervisor safety/priority/dwell logic.

## Next coherent sprint

Firmware bounded DTO reader and driver capability registry → validated signal fallbacks and semantic mapping → shared clock/targets snapshot → OutputSupervisor-safe output binding → separate NVS active/pending/known-good + rollback → active hash/readback/status → existing panel and e-ink observer. Hardware acceptance and revised typecheck/build gates are separate, not presumed completed.

Any continuation must start from fresh main and read `docs/CONFIGURATION_FLOW.md` and `docs/NEXT_CHAT.md`; no new branch, Codex, Work, or GitHub Actions.
