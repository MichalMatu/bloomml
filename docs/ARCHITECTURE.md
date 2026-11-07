# Architecture

Current state: [CURRENT_STATUS.md](CURRENT_STATUS.md).
Product order: [PROJECT_ROADMAP.md](PROJECT_ROADMAP.md).
Display port contract: [DISPLAY_UI_PORT.md](DISPLAY_UI_PORT.md).

## IPO v2 foundation boundary (unlinked)

`schemas/device-profile.v2.schema.json` and `web/src/domain/ipo-*.ts` define a bounded declarative device/signal/process/output/link graph; portable C++17 `lib/ipo_profile/src/IpoGraph.h` provides a typed validation seam and **symbolic** compatible-intent aggregator. None is linked into the production firmware. Hardware driver adapter/registry, safe profile NVS, apply/readback, and qualification remain open. Never bypass the existing `OutputSupervisor` or thermal interlocks; do not allow unqualified shadow ML to actuate hardware.

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

### NVS and durable-output ownership

Global ESP-IDF NVS lifecycle is owned by `RuntimeNvsOwner` in the runtime composition layer. It initializes NVS once before BLE and output persistence are started. Recovery erase for `ESP_ERR_NVS_NO_FREE_PAGES` / `ESP_ERR_NVS_NEW_VERSION_FOUND` is centralized there and is reported at boot because it resets durable state to safe defaults.

`BleClimateScanner` is an NVS consumer through NimBLE only. It must not initialize, erase or otherwise own the global NVS partition.

`RuntimePersistenceOwner` owns the durable output-policy/last-successful-command snapshot through `OutputNvsBackend`. When NVS is unavailable, the runtime keeps the configured shadow state store and safe output policy but leaves persistence disabled. A backend read error also keeps persistence disabled: safe defaults may be used for runtime policy, but an unreadable/unknown durable blob must not be treated as successfully loaded and later overwritten. A genuinely missing entry may initialize from safe defaults, while a decoded corrupt payload follows the explicit safe-default recovery path.

A failed persistence write does not mark the candidate snapshot as durable. The same snapshot remains eligible for retry. The real-input coordinator applies bounded exponential retry backoff (1 s, 2 s, 4 s ... capped at 60 s), rate-limits persistent-error logging and reports recovery after a later successful/unchanged synchronization.

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
 lib/growbox_display_model
     DisplayPageModel
          |
          v
 lib/growbox_clay_ui (C++20)
  Clay layout + monochrome raster
          |
          | writes only during open frame transaction
          v
 backend-owned 296x128 framebuffer
          |
          v
 async DisplayAsyncTransaction/service
          |
          v
 native SSD1680 backend / SPI / GPIO
```

`lib/growbox_display_model/` is a neutral C++17 data boundary only. It owns bounded page/line DTOs and must not acquire navigation, warning derivation, refresh cadence, rendering, hardware access, sensor truth or output/control ownership.

`lib/growbox_clay_ui/` owns Clay layout, clipping, text measurement and monochrome rasterization. It is isolated as C++20 because the pinned Clay 0.14 header requires C++20. Its public boundary is growbox-owned and C++17-compatible; no `Clay_*` type crosses into normal `src/` headers.

The production application remains C++17. `CrowPanelDisplayService` owns the existing async worker and one persistent Clay scratch arena allocated in PSRAM. `CrowPanelSsd1680DisplayBackend` remains the sole owner of the fixed 4736-byte framebuffer and all panel hardware. The backend exposes mutable framebuffer storage only while a frame transaction is open; Clay may fill it but may not retain or replace it.

`DisplayAsyncTransaction`, generation matching, refresh confirmation and retry behavior remain in the growbox display layer. Slow e-ink waits and transfers remain outside the control hot path. Observer state advances only after a successful render/backend completion reaches `confirmRendered()`.

Navigation remains `DisplayNavigation`; Clay does not own sensor/output truth, control policy, safety state, RF transport or hardware.

Phase 4 narrowed partial refreshes to the mapped dirty SSD1680 RAM window while preserving the single backend-owned framebuffer and refresh owner. The dirty baseline advances only after a physically successful render, and hardware/controller reinitialization falls back to a full refresh.

Native button input remains a semantic observer-side input path. Phase 6 adds only a read-only four-page chooser: temporary menu selection is owned by `DisplayMenuState`, while committed page ownership remains `DisplayNavigation` / `DisplayRuntimeController`. No menu path owns control policy, outputs, persistence, RF transport or safety state.

Production display changes must be verified with `make build-crowpanel`, which composes the CrowPanel N8R8, Stage27 NimBLE, Stage27C, `climate-v6-real-inputs` and e-ink configuration. Generic `make build` is not evidence that the production CrowPanel display path compiles.

## Configuration source of truth

Resolved runtime/build configuration is owned by CMake profiles under `config/` and exposed through generated `runtime/RuntimeBuildConfig.h`.

Production C++ must not reintroduce fallback `GROWBOX_*` default tables. Preprocessor definitions are retained only where compile-time preprocessing is actually required.

The canonical CrowPanel production build composes `config/idf/sdkconfig.defaults`, `sdkconfig.defaults.n8r8`, `sdkconfig.defaults.stage27` and `sdkconfig.defaults.stage27c` through `scripts/stage27c_crowpanel.sh`. The Stage27 overlay is required for the NimBLE host used by real BLE inputs.

## Climate-v6 controller core

`schemas/environment-controller.v6.json` and generated `ClimateContract.h` define the production climate-v6 controller contract. The portable core lives under `lib/environment_control/src/climate/`.

Within that core, `ClimatePolicy.*` owns stateless Rule request generation, capability arbitration, safety evaluation and request normalization. `ClimateRuntimeController.*` owns stateful trend/effective-action estimation, ML mode orchestration and reconciliation from execution truth. Keep these responsibilities separate; new Rule/safety policy should not be appended back into the runtime controller for convenience.

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
- canonical `make build-crowpanel` for production CrowPanel display work;
- other ESP-IDF profile builds/static analysis as appropriate;
- GitHub CI;
- hardware qualification only when a fresh physical executable claim is required.

Simulator/host/build PASS is software evidence, not physical acknowledgement.

## IPO Runtime Bridge 1 — integration without output ownership transfer (2026-11-07)

Production `RuntimeScheduleProfileOwner` is an in-memory validated snapshot holding the built-in Mint values. `ConfiguredScheduleConfigSource` for Rule and `buildScheduleIntent(..., profile)` for lamp schedule consume the **same configuration owner**, not independent hard-coded constants. The owner does not parse browser JSON or load NVS yet. A read-only `status` line reports generation/default Mint and marks `persisted=0` and `profile_v2_applied=0`.

`lib/ipo_profile/src/IpoBoardAdmission.h` is a read-only CrowPanel driver and fixed RF433 reference admission layer; the firmware bootstrap logs the admission result with `apply_authorized=false`. `IpoSignalSelector.h` is a bounded freshness/priority helper, not a runtime sensor authority yet. Physical output path still belongs solely to `OutputSupervisor`, existing safety and fixed RF433 endpoints. The code has **not been compiled or physically qualified** in this sprint.

## IPO full-profile durable storage (code only; inactive)

`src/climate/runtime/profile/IpoProfileWire.h` defines an explicit bounded wire representation of the entire typed IPO graph and accepted schedule targets, with schema version/magic/generation/CRC32 and board admission. `IpoProfileNvsJournal` uses an independent NVS namespace `gb_ipoprof`, alternating slots and a separate active marker to verify a candidate before switching. Read errors and corruption must not silently become a successful active profile or trigger output writes; recovery is reported, not self-promoted. This store never initializes/erases global NVS. Source is included in real-input CMake but has **no current runtime caller**, NVS transaction coordinator or authority to install into the controller. It is not tested/compiled in this sprint. Future multi-task access needs explicit single-owner serialization and device-side tests before using `commit`.

## Strict browser v2 → firmware execution snapshot boundary (code only)

`IpoProfileJsonDecode` performs length/schema/exact key checks, canonicalizes symbolic device/process/actuator IDs, validates channel/role/process graphs, validates CrowPanel driver+physical endpoint restrictions, then produces a bounded typed `DurableIpoProfile`. It does not expose external apply. The encoded state intentionally excludes free-text UI labels and string IDs, retaining only execution semantics; any full JSON round-trip requires separate metadata storage. `IpoProfileStagingCoordinator` is not instantiated; when later introduced, a single serialized runtime owner must coordinate preview, NVS, install, readback and rollback. Firmware `status.schedule_hash` fingerprints the currently running *Mint schedule*, not a full v2 graph or confirmation of physical endpoint state. This code remains untested/unqualified.


## IPO reference signal adapter / inactive stored profile (Bridge 4)

`RuntimeInsideSource` and `RuntimeNearbySource` receive the compiled-in `makeCrowPanelMintReferenceGraph()`, then use `IpoSignalSelector` to normalize **only existing supported physical** measurements. Each selected role is gated by valid/finite/age and the current in-RAM schedule timeout; no unqualified fallback, and lamp's direct TP357 Safety remains separate. The existing source constructors without graph remain for backward compatibility. The IPO NVS journal is read at boot solely for diagnostics: `stored` does **not** imply `active`. Console readback distinguishes `ipo_store` vs `ipo_profile` vs `ipo_input`; no GUI transport or physical state acknowledgement is added. Source-level integration remains uncompiled and hardware-unqualified, particularly its effects on Rule validity transitions.

## B5 — readback evidence and strict current-board admission

A DeviceProfile v2 graph may be valid for future drivers while remaining incompatible with the current compiled CrowPanel. Browser `ipo-board-readiness.ts` is an observer-only approximation of current `IpoBoardAdmission`, never hardware discovery or apply authority. CrowPanel requires a present CO₂ input and continuous qualified exhaust path in addition to the known RF433 role routes. The offline parser's 32 KiB maximum JSON byte budget stays separate from the 1536-byte typed NVS wire envelope; parsing is dormant outside the control hot path. `IpoJournalReadResult` now holds a CRC32 fingerprint of validated on-disk bytes computed during boot load (not while printing console status), and `IpoProfileReadback` reports NVS truth separately from RAM Mint. `status` adds readable observer-only `ipo_health`/`ipo_readback` lines, no output owner changes. New targeted host/Vitest cases have been authored but **not executed**; physical activation is still gated.

## IPO v2 transactional activation boundary (2026-11-07)

`src/climate/runtime/profile/IpoProfileActivationTransaction.h` is the **single serialized transaction owner**, not a control or output engine. The typed graph admission and NVS journal remain separate. The owner can swap only `RuntimeScheduleProfileOwner` (Rule and lamp schedule) while an independent safety gate holds the existing `OutputSupervisor` interlock; neither persistence nor console, display, ML or parser may write configured physical outputs. Stored NVS remains distinct from active RAM and independently verified activation evidence. Failure to restore both RAM and NVS demands a latched output quarantine.

Production currently injects `ClosedIpoActivationGate`, which **always refuses activation**. There is no privileged apply caller and no automatic apply on boot. An exact-SHA hardware-qualified gate, with a real `OutputSupervisor` quiesce/lock and independent lamp safety proof, remains required before enabling physical activation. Host fault-injection tests are not hardware authorization.
