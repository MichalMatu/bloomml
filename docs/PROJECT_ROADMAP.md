# Growbox ML project roadmap

Updated: 2026-09-21

## Direction

The product is a native ESP-IDF ESP32-S3 growbox controller using real sensors, deterministic Rule control, RF433 outputs, durable telemetry, e-ink operator visibility and an ML shadow/research path.

Broad architecture cleanup, the SCD41/e-ink recovery closeout, runtime persistence hardening and Clay display Phases 1-6 are integrated. Prefer bounded changes that preserve ownership, safety and ESP32-S3 resource limits.

## Active work

### 1. Clay/menu/button/simulator display stage

Use `DISPLAY_UI_PORT.md` as the implementation contract.

Current position:

1. isolated C++20 Clay component and exact 296x128 host simulator — DONE;
2. reproduce current Environment / Outputs / System / Diagnostics pages on host — DONE;
3. connect Clay behind the existing async display transaction and backend-owned framebuffer — DONE;
4. implement true dirty-region RAM-window partial transfer — DONE;
5. native ESP-IDF button stack — IMPLEMENTED, but physical key qualification is OPEN;
6. bounded read-only growbox page chooser — IMPLEMENTED, but depends on qualified physical keys;
7. exact-SHA physical qualification — PARTIAL: autonomous boot/display/resource evidence is green; physical-key and SCD41-with-device-present stabilization remain OPEN.

Current priority is stabilization, not UI expansion. First prove all five physical keys with the known-good CrowPanel mapping and event semantics, then qualify SCD41 with the device attached. Only after those pass should menu/settings work resume.

Do not replace the native SSD1680 backend, create another framebuffer/refresh owner or make UI another output owner. Production display integration must be verified with `make build-crowpanel`; the generic default firmware build does not compile the CrowPanel real-input display path.

### 2. Controller behavior quality

Build a replay/telemetry baseline from real growbox data, quantify temperature/humidity and absolute-humidity ventilation behavior, then choose one tuning candidate with explicit acceptance criteria.

### 3. Configuration and operator UX

Reduce friction in sensors, targets, schedules, outputs and growbox parameters without inflating ESP32-S3 RAM/CPU/flash cost. UI writes must route through existing application/domain ownership.

### 4. Logging, history and explainability

Make it easy to answer what the controller observed, why it chose an action, what transport attempted, and how the environment responded.

### 5. ML shadow/research quality

Improve datasets, replay metrics and Rule-vs-ML comparison. ML stays non-authoritative until separately justified and qualified.

### 6. Justified device expansion

Add integrations only when a concrete growbox use case justifies implementation and maintenance cost.

## Architecture quality rule

Before substantial implementation, identify the owning module, state owner, dependency direction, side effects, ESP32-S3 resource impact and focused verification. Do not grow central coordinators/controllers merely because they already see the required state. The repository-wide implementation gate and oversized-file watchlist live in `../AGENTS.md`.

## Closed work that should not be reopened by default

- SCD41 clean-start/liveness recovery design is retained, but physical SCD41 qualification is currently open;
- CrowPanel SSD1680 pin map and native backend ownership;
- display rotation and async worker architecture;
- runtime NVS/output-persistence hardening;
- isolated Clay component and exact host simulator;
- semantic four-page Clay reproduction and deterministic golden coverage;
- Clay production integration behind the existing async transaction and single backend-owned framebuffer;
- true dirty-window partial transfer, native semantic buttons and the bounded read-only page chooser;
- Rule-authoritative production ownership;
- output truth separation and `OutputSupervisor` ownership.

Reopen only on new evidence, not because an older handoff or research document mentions the old issue.

## Selection rule

Rank work by practical growbox value, safety risk, ownership clarity, ESP32-S3 resource cost, verification coverage and reversibility.

## Branch/work policy

- `main` — canonical source/documentation;
- `agent-control` — Local Agent control only;
- `gh-pages` — publishing only;
- temporary branches are disposable after verified integration.

For each new task start from `docs/README.md`, `CURRENT_STATUS.md` and the relevant subsystem contract.
