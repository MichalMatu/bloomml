# Architecture

Current state: [CURRENT_STATUS.md](CURRENT_STATUS.md).
Product order: [PROJECT_ROADMAP.md](PROJECT_ROADMAP.md).
Display port contract: [DISPLAY_UI_PORT.md](DISPLAY_UI_PORT.md).

## Stable design rules

The portable climate controller is independent from concrete sensor libraries and physical actuator transports. Hardware code produces semantic measurements and consumes semantic output intent through explicit application/runtime boundaries.

Normal configured physical output execution has one owner: `OutputSupervisor`.

Production deterministic Rule control remains authoritative. ML may be evaluated in shadow/research modes but is not qualified as production actuation authority.

## Production real-input path

```text
native sensors / RTC / schedule sources
               |
               v
     ClimateApplication / climate-v6
               |
        ControlIntent + ScheduleIntent
               |
               +----------------------+
               |                      |
               v                      v
       Lamp SafetyEnvelope      maintenance lifecycle
               |                      |
               +----------+-----------+
                          v
                  OutputSupervisor
              resolver + binary policy
                          |
                          v
                 OutputPlan / command
                          |
                          v
               RuntimeOutputTransport
                          |
                          v
                 RF433OutputTransport
```

One-way RF transport completion is command/transport evidence only; it is never physical acknowledgement.

## Runtime composition

The real-input runtime is split by responsibility:

- `ClimateV6RealInputRuntime.cpp` — thin bootstrap;
- `runtime/core/RealInputRuntimeComposition.*` — construction, ownership and lifetime wiring;
- `runtime/core/RealInputRuntimeCoordinator.*` — one-cycle orchestration;
- `runtime/core/RuntimeCycleState.*` — bounded sequencing/cadence state;
- `runtime/core/RuntimeOutputTransport.*` — physical transport availability/truth boundary;
- `runtime/telemetry/*` — runtime/output telemetry;
- `runtime/console/*` — service-console routing/handlers;
- `runtime/diagnostics/*` — passive diagnostics;
- `output/*` — configured-output ownership/policy/execution;
- `rf433/*` — RF protocol and transport.

Invalid lifecycle/automation/maintenance reports fail closed by disabling physical transport readiness for the cycle rather than being silently discarded.

## Output truth model

Requested, resolved, attempted/executed transport state and independently observed physical state are distinct concepts.

When physical transport is unavailable, `RuntimeOutputTransport` returns `NotAttempted`/`Unavailable`. It must not manufacture a completed execution state.

Raw RF remains an explicit maintenance capability behind the maintenance boundary; it is not a hidden normal-output path.

## Policy and safety ownership

- climate Rule logic owns environmental control decisions;
- `BinaryActuatorPolicy` owns binary hysteresis/deadband/dwell behavior;
- output bindings own endpoint/policy mapping;
- lamp thermal safety owns the frozen `>=28 C` trip and `<=26 C` for 10 minutes recovery contract;
- transport layers do not own climate policy;
- maintenance diagnostics do not become normal automation.

## Display/UI architecture

The e-ink path is an observer, not a controller:

```text
runtime/output/sensor truth
          |
          v
     DisplaySnapshot
          |
          v
    DisplayPresenter
          |
          v
 semantic page/layout data
          |
          v
 renderer / framebuffer
          |
          v
 async display service
          |
          v
 native SSD1680 backend
```

Slow e-ink waits and transfers remain outside the control hot path. Observer state advances only after successful rendering according to the existing transaction contract.

The next Clay stage must preserve that ownership. Clay may own layout/clipping/menu visual state but must not own sensor truth, output truth, RF transport or safety state.

### C++ boundary for Clay

Current application code is C++17. The pinned Clay 0.14 donor header requires C++20. The preferred architecture is a separate C++20 component with growbox-owned C++17-compatible input/output structs and no public `Clay_*` types. See `DISPLAY_UI_PORT.md`.

## Configuration source of truth

Resolved runtime/build configuration is owned by CMake profiles under `config/` and exposed through generated `runtime/RuntimeBuildConfig.h`.

Production C++ must not reintroduce fallback `GROWBOX_*` default tables. Preprocessor definitions are retained only where compile-time preprocessing is actually required.

## Climate-v6 controller core

`schemas/environment-controller.v6.json` and generated `ClimateContract.h` define the production climate-v6 controller contract. The portable core lives under `lib/environment_control/src/climate/` and contains feature encoding, Rule/ML evaluation, trend estimation and the portable control loop.

Production real-input composition is statically configured for Rule authority with unqualified ML active control disabled.

The broader v4 `schemas/environment-controller.json` / 128-feature / 15-output contract belongs to older simulator/tooling workflows and is not the production runtime contract. See `DATA_CONTRACT.md` for the distinction.

## Legacy isolation

Legacy controller/demo code remains available only through explicit `legacy` app mode. Production V6 targets do not compile the legacy controller ownership path by default.

`src/main.cpp` is an app-mode dispatcher; it does not own production control orchestration.

## Verification layers

- architecture/config ownership guards;
- focused portable regression tests;
- host C++ suites;
- Python scientific/replay tests where relevant;
- simulator/golden checks for UI work;
- ESP-IDF production builds/static analysis;
- GitHub CI;
- hardware qualification only when a fresh physical executable claim is required.

Simulator/host/build PASS is software evidence, not physical acknowledgement.
