# BloomML — product direction and architecture boundary

Decision date: 2026-11-07. This document describes the intended product and current limitations; it does not authorize ML to actuate hardware.

## IPO v2 evolution (2026-11-07)

The operator homepage now edits a bounded, draft-only **DeviceProfile v2**, with device/driver instances, prioritized signals, typable processing, output roles and many-to-one links. Fixed v1 is preserved at `/lab/profile-v1`; v5 research at `/lab/v5`. Actual hardware still uses fixed Mint and three RF433 outputs. This is not a firmware apply/readback or active ML feature. Details and capability truth are in `CONFIGURATION_FLOW.md`.

## Product promise

An operator should be able to:

1. Describe the real growbox hardware: sensors, physical outputs, mappings, capabilities and availability.
2. Choose environmental targets, schedules and safety limits.
3. Start autonomous control without choosing a training algorithm, simulator, model file or firmware build profile.
4. See current readings, decisions, execution evidence, alarms and safe fallback state.

The machine does the rest **only within proven capabilities**. A transparent deterministic controller remains the safe production fallback; ML/MPC earns control authority through evidence, not through a UI toggle.

## One product, three deliberately separate surfaces

| Surface | Intended audience | Current implementation | Direction |
| --- | --- | --- | --- |
| Production setup + status | Growbox operator | **One offline v1 setup draft** on browser `/`; firmware still fixed Mint and e-ink observer-only | Add safe profile apply/persistence/readback and live status through existing runtime owners |
| Local hardware/demo service | Developer/technician | `tools/panel/`, serial protocol with **research v4** scenario controls | Developer diagnostics only; never represent as the production configuration backend |
| Research v5 + 3D | Research/design | `web/` now keeps v5 on `/lab/v5`; `/chamber-3d` is optional independent geometry | Keep as clearly marked research/design surfaces, not a physical simulation or device backend |
| Physics / ML / MPC lab | Research/development | `tools/ml/`, simulator, replay, Kaggle and runtime-isolated MPC | Offline qualification, training, failure analysis; no runtime dependency |
| E-ink | Device operator | `src/climate/display/` | Status/diagnostics observer; not a control-authority owner |

**Never silently convert a v4/v5 research JSON export into the v6 production runtime.** The present schema families are different, with distinct feature/output counts and meanings:

- Research v4: `schemas/environment-controller.json`, 128 features / 15 outputs.
- Browser research v5: `web/schema/environment-controller.v5.json`, 228 features / 25 outputs.
- Production climate-v6: `schemas/environment-controller.v6.json`, 44 ML features / 6 climate outputs. This is an inference contract, **not yet a complete operator device-profile API**.

The v1 **draft** of the missing product piece is now a small, versioned, validated **operator device profile** (real sensor identity/role, output role/transport, targets, schedules, physical limits). It must be mapped through existing application/runtime/output owners, not implemented by sharing experimental schemas or exposing raw RF. The offline draft UI exists; the firmware adapter and fresh tests/build qualification remain open. See [CONFIGURATION_FLOW.md](CONFIGURATION_FLOW.md) before any additional implementation.

## Runtime authority and safety

```text
validated operator profile + real measurements + schedules
                     |
          deterministic Rule fallback
                     |
     optional, measured ML/MPC proposal
                     |
      safety/availability/arbitration
                     |
     OutputSupervisor -> transport
                     |
       execution evidence + logging
```

- Rule control is currently authoritative. ML is shadow/research-only on `main`.
- `OutputSupervisor` is the single configured-output owner. An ML proposal must not bypass safety, thermal interlocks, real sensor validity, persistence or execution-truth checks.
- Missing/stale sensors, unknown output state, poor model confidence or timed-out inference must fail safely.
- ESP32-S3 remains autonomous/offline. Mac/Kaggle/GPU are training and development infrastructure only, never a production control dependency.
- The preserved 2026-11-04 MPC research has a limited success case but **19/22 hard scenario failures**. Safety retest on the latest research patch is not yet independently confirmed. Do not enable MPC on hardware from these results.

## Work ordering: product before experiments

**P0 — Establish the contracts (documentation + tests, no new UI):**
- Inventory current production sensor/output capabilities and existing operator parameters; identify what is configurable and what is still compiled-in.
- Design one versioned `DeviceProfile` boundary and backward-compatible migration story. Validate physical identifiers, units, ranges, unavailable modules, and dangerous combinations; retain immutable runtime safety limits.
- Agree on one user-facing setup/status experience; keep demo and research interfaces explicitly labeled until a real integration exists.

**P1 — End-to-end operator path:**
- Build one minimal slice: sensor role -> output role -> desired targets -> save/apply -> verify observed state -> restart/recovery. No speculative panel rewrite.
- Prove that the same profile is reflected in diagnostics; distinguish requested, transport-attempted and independently observed output truth.
- Hardware acceptance on the authorized physical board and sensors is a separate gate.

**P2 — Telemetry and real-world evidence:**
- Record time-stamped inputs, validated applied outputs, targets, alarm/fallback causes and environment response from the user's own growbox.
- Calibrate an explainable dynamics model with train/validation/test split by trajectory/time, with provenance.
- Preserve simple Rule, persistence and optionally PID baselines.

**P3 — ML optimization as an earned capability:**
- First shadow forecasting/control and counterfactual replay; then software-in-the-loop, long-run fault testing and bounded MCU performance qualification.
- Require demonstrated improvements against Rule/PID on held-out real trajectories, safe fallback, deterministic bounded inference, memory/time limits and independent hardware sign-off.
- Promote to active only after explicit review. Model choice/training remains invisible to ordinary operators.

## Immediate non-goals

The 110-commit research history is now merged into `main`, but the research remains runtime-isolated. No forced integration of v4/v5/v6; no deletion of panels or old evaluation evidence; no new model training solely to increase scores; no new cloud/runtime dependency; no new firmware output authority.

See `AUDIT_STATUS.md` for what has been tested in this audit, and `NEXT_CHAT.md` for the next bounded, closable sprint.
