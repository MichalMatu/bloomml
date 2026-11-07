# BloomML — next step after configuration profile draft (2026-11-07)

## Read this first

- `AGENTS.md`, `docs/CONFIGURATION_FLOW.md`, `docs/CURRENT_STATUS.md`, `docs/ARCHITECTURE.md`, `docs/PRODUCT_DIRECTION.md`.
- Fetch fresh `main` and `agent-control:.agent/status/daemon.json` before any write or Local Agent operation.
- Development only on `main`. Retain `agent-control` for Local Agent and `gh-pages` for publication. No Codex, Work or GitHub Actions.
- Do not redo old tests, heavy builds, hardware gates or repeat the 110-commit research merge. Current operator expressly asked to defer those.

## Completed this sprint

- One canonical map: `docs/CONFIGURATION_FLOW.md`, deriving the actual sensor, actuator and environmental parameters from firmware source.
- Draft schema `schemas/device-profile.v1.schema.json`, sample `profiles/crowpanel-mint.v1.json`, strict offline draft validator/editor.
- Browser `/` = simple profile; `/lab/v5` = original 228-field research editor; `/chamber-3d` unchanged preview.
- Profile can be imported and exported as `draft_not_applied`; it **cannot be sent or applied to the ESP32-S3**. Do not claim runtime integration or real live status.
- Production Rule, RF433, NVS output supervisor and e-ink behavior unchanged. No tests/compilations executed for this new source; software status **UNVERIFIED**.

## Single next sprint: safe runtime application

1. Introduce a typed, bounded, versioned production profile parser/validator using `schemas/device-profile.v1.schema.json`; do not treat JSON as an ML v4/v5/v6 feature vector.
2. Keep actual fixed RF433 endpoints and sensor sources until a safe remapping mechanism exists. Feed validated targets/clock/schedule/capability flags into existing runtime ownership, sharing exactly one snapshot for schedule lamp + climate targets.
3. Implement separate profile NVS persistence with version/recovery/rollback; preserve existing output state/policy NVS, strict safety and Rule authority.
4. Add readback of active applied profile hash, actual sensor validity/age, desired Rule and shadow proposal, applied vs attempted output, fault state. Only after real device API exists attach the existing browser page's preview/apply/readback and status; keep e-ink observer-only.
5. New physical qualification remains explicitly OPEN until separately authorized. Avoid repetitive broad gates.

## Known limitations

SCD41 is currently used for CO₂ **only**; T/RH come from TP357 BLE. Xiaomi BLE supplies outside T/RH; DS3231 supplies time. Runtime has three RF433 roles: exhaust, scheduled light, humidifier. CO₂ dose, heater/cooler/dehumidifier, PPFD/DLI, irrigation and active MPC are **not** working production features. MPC research still has 19/22 hard-case FAILs and no empirical growbox calibration.
