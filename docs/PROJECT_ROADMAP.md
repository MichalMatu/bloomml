# Growbox ML project roadmap

Updated: 2026-09-28

## Direction

The product is a native ESP-IDF ESP32-S3 growbox controller using real sensors, deterministic Rule control, RF433 outputs, durable telemetry, e-ink operator visibility and an ML shadow/research path.

Broad architecture cleanup, the SCD41/e-ink recovery closeout, runtime persistence hardening and Clay display Phases 1-6 are integrated. Prefer bounded changes that preserve ownership, safety and ESP32-S3 resource limits.

The deployed product must remain standalone on the ESP32-S3. External foundation models, training services or cloud inference may be used only in offline research/development and must not become runtime dependencies.

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

This real/replay baseline is also the prerequisite for the TimesFM teacher experiment below. Do not use another broad synthetic-only training pass as a substitute for real or calibrated trajectories.

### 3. Configuration and operator UX

Reduce friction in sensors, targets, schedules, outputs and growbox parameters without inflating ESP32-S3 RAM/CPU/flash cost. UI writes must route through existing application/domain ownership.

### 4. Logging, history and explainability

Make it easy to answer what the controller observed, why it chose an action, what transport attempted, and how the environment responded.

### 5. ML shadow/research quality

Improve datasets, replay metrics and Rule-vs-ML comparison. ML stays non-authoritative until separately justified and qualified.

#### TimesFM teacher / forecasting track

Use Google Research TimesFM only as an offline research teacher and benchmark. TimesFM must never be required by the deployed controller; the final candidate must execute locally on the ESP32-S3 with no network, server, Raspberry Pi or cloud dependency.

Ordered plan:

1. **Freeze the forecasting task from real/replay data.** Define a leakage-free history window, target signals and useful control horizons. Initial candidates are inside temperature, relative humidity and CO2 at +5, +15, +30 and +60 minutes, refined from measured growbox dynamics rather than assumed synthetic behavior.
2. **Create a reproducible evaluation dataset.** Reuse deterministic runtime traces and calibrated replay trajectories. Preserve time ordering and split by trajectory/time so nearby samples from the same run cannot leak across train/test boundaries. Record relevant observed actuator state and known exogenous/time features separately from forecast targets.
3. **Establish simple baselines first.** Compare persistence and a small growbox-native predictor with the existing climate-v6 research path before attributing value to a foundation model.
4. **Evaluate TimesFM offline.** Run the current suitable TimesFM checkpoint on the same held-out trajectories and horizons. Treat it as a forecaster, not a causal controller: historical actuator state may be context, but hypothetical future actuator commands must not be interpreted as valid counterfactual effects without supporting data/modeling.
5. **Attempt teacher-to-student transfer only on evidence.** If TimesFM materially improves held-out forecast quality or useful threshold-crossing prediction, train/distill a bounded student model that fits the ESP32-S3 resource envelope. Prefer the smallest architecture that preserves the measured gain; do not copy the foundation model architecture onto the MCU.
6. **Integrate the student in shadow mode.** Add only the minimal firmware inference boundary needed to consume existing observations and emit forecasts/diagnostics. Rule control and safety remain authoritative; forecast output must not directly own physical outputs.
7. **Qualify embedded cost and behavior.** Measure flash, internal RAM/PSRAM, inference latency, control-loop impact and failure behavior on the production CrowPanel profile. Compare embedded student predictions against the Python reference on fixed vectors and then against held-out/physical trajectories.
8. **Decide whether forecasting earns a product role.** Promote nothing beyond shadow/research without explicit acceptance criteria showing practical growbox value over the simple baselines and without a separate safety/architecture/hardware qualification.

Research/tooling boundaries:

- keep TimesFM dependencies/checkpoints outside production firmware and outside normal device startup/build requirements;
- do not vendor TimesFM 3.0 weights into firmware artifacts; its current pretrained-weight license is research/non-commercial and must remain isolated from any future production/commercial path;
- keep generated datasets, teacher predictions and training/evaluation tooling under research/tool ownership rather than production control ownership;
- the useful output of this track is an ESP32-sized student plus reproducible evidence, not a runtime connection between the ESP32 and TimesFM.

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
