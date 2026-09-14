# Growbox ML project roadmap

Updated: 2026-09-14

## Direction

The product is a native ESP-IDF ESP32-S3 growbox controller using real sensors, deterministic Rule control, RF433 outputs, durable telemetry, e-ink operator visibility and an ML shadow/research path.

Architecture cleanup is finished. Avoid broad rewrites unless concrete evidence requires them. Prefer small measurable improvements that preserve ownership and safety boundaries.

## Active work

### 0. SCD41 regression

Restore successful SCD41 sampling on the current CrowPanel firmware before changing display semantics or controller behavior.

Acceptance:

- root cause identified from evidence;
- smallest coherent fix;
- regression coverage;
- exact candidate builds and flashes;
- real SCD41 samples appear on `/dev/cu.usbserial-1130`;
- no new panic/watchdog/reset/memory regression;
- physical outputs remain disabled during unattended verification.

### 1. Finish e-ink operator UI

After SCD41 is healthy:

- show authoritative environment values;
- finish bounded Environment / Outputs / System / Diagnostics presentation;
- port only the useful Clay/menu/button/simulator pieces from the proven LiteGraph implementation;
- preserve observer-only ownership and output-truth semantics;
- qualify the final exact firmware SHA on hardware.

### 2. Controller behavior quality

Build a replay/telemetry baseline from real growbox data, quantify temperature/humidity and absolute-humidity ventilation behavior, then choose exactly one tuning candidate with explicit acceptance criteria.

### 3. Configuration and operator UX

Reduce friction in sensors, targets, schedules, outputs and growbox parameters without inflating ESP32-S3 RAM/CPU/flash cost.

### 4. Logging, history and explainability

Make it easy to answer what the controller observed, why it chose an action, what transport attempted, and how the environment responded.

### 5. ML shadow/research quality

Improve datasets, replay metrics and Rule-vs-ML comparison. ML stays non-authoritative until separately justified and qualified.

### 6. Justified device expansion

Add integrations only when a concrete growbox use case justifies implementation and maintenance cost.

## Selection rule

Rank work by:

1. practical growbox value;
2. safety risk;
3. ownership clarity;
4. ESP32-S3 resource cost;
5. verification coverage;
6. change size and reversibility.

## Branch/work policy

- `main` — canonical source;
- `agent-control` — Local Agent control only;
- `gh-pages` — publishing only;
- temporary feature branches are disposable after verified integration.

For current execution details read `AGENTS.md` and `docs/CURRENT_STATUS.md`.
