# BloomML — product direction and architecture boundary

Decision date: 2026-11-07. This document describes the intended product and current limitations; it does not authorize ML to actuate hardware.

## IPO v2 evolution (2026-11-07)

The operator homepage now edits a bounded, draft-only **DeviceProfile v2**, with device/driver instances, prioritized signals, typable processing, output roles and many-to-one links. Fixed v1 is preserved at `/lab/profile-v1`; v5 research at `/lab/v5`. Actual hardware still uses fixed Mint and three RF433 outputs. The typed transaction exists in firmware source and diagnostics show stored vs active, but production `ClosedIpoActivationGate` prevents apply. The offline draft is not active ML or hardware configuration. Details and capability truth are in `CONFIGURATION_FLOW.md`.

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
| Production setup + status | Growbox operator | **One offline IPO v2 setup draft** on browser `/`; firmware still fixed Mint and e-ink observer-only | Add safe profile apply/persistence/readback and live status through existing runtime owners |
| Local hardware/demo service | Developer/technician | `tools/panel/`, serial protocol with **research v4** scenario controls | Developer diagnostics only; never represent as the production configuration backend |
| Research v5 + 3D | Research/design | `web/` now keeps v5 on `/lab/v5`; `/chamber-3d` is optional independent geometry | Keep as clearly marked research/design surfaces, not a physical simulation or device backend |
| Physics / ML / MPC lab | Research/development | `tools/ml/`, simulator, replay, Kaggle and runtime-isolated MPC | Offline qualification, training, failure analysis; no runtime dependency |
| E-ink | Device operator | `src/climate/display/` | Status/diagnostics observer; not a control-authority owner |

**Never silently convert a v4/v5 research JSON export into the v6 production runtime.** The present schema families are different, with distinct feature/output counts and meanings:

- Research v4: `schemas/environment-controller.json`, 128 features / 15 outputs.
- Browser research v5: `web/schema/environment-controller.v5.json`, 228 features / 25 outputs.
- Production climate-v6: `schemas/environment-controller.v6.json`, 44 ML features / 6 climate outputs. This is an inference contract, **not yet a complete operator device-profile API**.

The v2 **draft** of the operator setup is a bounded, versioned **device/signal/process/output graph** (real sensor identity/role, output role/transport, targets, schedules, physical limits). It must be mapped through existing application/runtime/output owners, not implemented by sharing experimental schemas or exposing raw RF. The offline v2 draft UI, a strict firmware parser, read-only IPO NVS boot inspection, and a fixed-board signal selector exist in source; safe active apply and physical device qualification remain open; the real-input CrowPanel ESP-IDF build passed at source `1a1b3de8`. See [CONFIGURATION_FLOW.md](CONFIGURATION_FLOW.md) before any additional implementation.

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

## Product sequencing

IPO v2 draft/host and CrowPanel compilation are complete. **Next is the authorized CrowPanel physical connection plus output isolation; only afterward physical acceptance, supervisor-owned lock and authenticated apply API.** See [PROJECT_ROADMAP.md](PROJECT_ROADMAP.md) and [NEXT_CHAT.md](NEXT_CHAT.md). Real-data telemetry and ML shadow qualification follow a proven operator path, not vice versa.

## Immediate non-goals

The 110-commit research history is now merged into `main`, but the research remains runtime-isolated. No forced integration of v4/v5/v6; no deletion of panels or old evaluation evidence; no new model training solely to increase scores; no new cloud/runtime dependency; no new firmware output authority.

See `AUDIT_STATUS.md` for what has been tested in this audit, and `NEXT_CHAT.md` for the next bounded, closable sprint.
