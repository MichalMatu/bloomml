# Agent notes — Growbox ML

## Repository identity

- repository: `MichalMatu/growbox-ml-controller`
- repository id: `growbox-ml-controller`
- agent binding: `815cf40f-8d2a-4e1f-b7cc-c0f4e37b6cb5`
- source branch: `main`
- control branch: `agent-control`
- publishing branch: `gh-pages`

Do not infer or substitute another repository. A different repository requires an explicit Chat Bridge rebind.

## Bootstrap

For a fresh task read, in order:

1. `AGENTS.md`
2. `docs/README.md`
3. `docs/CURRENT_STATUS.md`
4. `docs/ARCHITECTURE.md`
5. `docs/PROJECT_ROADMAP.md`
6. the specific subsystem document for the change; current display work uses `docs/DISPLAY_UI_PORT.md`

Then fetch fresh `main` and `agent-control:.agent/status/daemon.json` before any write or Local Agent task.

Historical Stage27/Stage28 handoffs, temporary prompts and superseded audits are not live documentation. Use `docs/HISTORY.md`, `docs/CHANGELOG.md` and Git history when historical evidence is needed.

Research documents such as `DATA_CONTRACT.md`, `CONFIG_MATRIX.*` and `docs/simulator/*` may describe the older v4 128-feature/15-output toolchain. Do not treat them as the production climate-v6 runtime contract unless the task explicitly targets that research toolchain.

## Work mode

Use direct GitHub for bounded source/config/documentation changes when the exact diff and CI can verify the result.

Use ChatGPT Sandbox for repository-only execution when matching source/dependency packs exist. Canonical details are in `docs/SANDBOX_EXECUTION_FLOW.md`.

Use Local Agent for Mac-local toolchains, local simulator assets not represented in the sandbox, USB/serial, flashing, physical-board observation and other machine-specific evidence.

Every executable Local Agent task for this repository must contain exactly:

```json
{
  "agent_binding": "815cf40f-8d2a-4e1f-b7cc-c0f4e37b6cb5",
  "work_branch": "main",
  "resources": []
}
```

Task ids/payloads are immutable. Read terminal `.agent/results/<task-id>.json` before claiming success. Never launch or delegate to local Codex from a Local Agent task.

`agent-control` is control-plane state only; never use it for product code. Do not race direct writes against an active Local Agent task on the same source branch.

## Architecture-first implementation gate

For every non-trivial change, determine the architecture before writing implementation code.

Before editing, identify:

- the responsibility being added or changed;
- the owning layer/module from `docs/PROJECT_LAYOUT.md`;
- the single owner of mutable state;
- inputs/outputs and dependency direction;
- hardware, storage, network and UI side effects;
- ESP32-S3 RAM/CPU/flash/hot-path impact where applicable;
- the smallest focused tests that can prove the behavior.

Do not attach a new responsibility to an existing class merely because it already has convenient access to the required state. If the feature introduces an independently changing responsibility, create or extend a focused module and keep the existing coordinator/composition object thin.

For substantial work use this order:

`UNDERSTAND -> DESIGN BOUNDARIES -> IMPLEMENT MINIMAL VERTICAL SLICE -> TEST -> REVIEW ARCHITECTURE -> SIMPLIFY -> COMPLETE`

Do not intentionally land a shortcut with a plan to clean it up later unless the operator explicitly requests a temporary experiment.

## Dependency and ownership rules

Keep dependency direction explicit:

- `lib/environment_control/` is portable domain/control code and must not depend on ESP-IDF, hardware drivers, UI, filesystem or `src/` runtime code;
- `src/climate/runtime/core/RealInputRuntimeComposition.*` owns construction/lifetime wiring, not control policy;
- `src/climate/runtime/core/RealInputRuntimeCoordinator.*` owns cycle orchestration, not transport protocol, persistence implementation or UI rendering;
- `src/climate/output/` owns configured-output policy/execution; normal configured physical output ownership stays with `OutputSupervisor`;
- `src/climate/rf433/` is transport/protocol and must not acquire climate policy;
- `src/climate/display/` and Clay UI remain observers and must not become control/output owners;
- storage/persistence code stores durable state but does not decide climate policy;
- `tools/` and research ML code do not become production runtime authority by convenience.

Do not introduce circular dependencies to avoid extracting a small shared model. Prefer a narrow growbox-owned data boundary over leaking framework/library types across layers.

## Anti-God-object and cohesion rules

Every class/module must have one primary reason to change.

Do not create or grow:

- manager-of-everything or controller-of-everything objects;
- mutable global state containers;
- utility dumping grounds;
- UI handlers containing domain/control logic;
- transport classes containing policy;
- persistence classes containing orchestration;
- giant switch/if chains that mix unrelated subsystem responsibilities.

Treat these as architecture warnings, not automatic failures:

- production function >= 80 lines;
- class >= 250 lines;
- production source file >= 500 lines;
- constructor requiring more than 7 collaborators;
- one module exposing unrelated groups of methods.

Existing oversized files are not permission to keep growing them. Current watchlist from the 2026-09-20 architecture audit:

- `lib/environment_control/src/SafetySupervisor.cpp` — safety policy only; new independent safety domains should be extracted into focused helpers/policies rather than appended indefinitely;
- `tools/panel/static/js/form.js` — form binding/orchestration only; new panel behavior should prefer focused JS modules;
- `src/demo/protocol/ScenarioWireCodec.cpp` — wire codec only; do not add simulation/control ownership.

Generated files, pinned third-party/vendor code and large test fixtures are exempt from size heuristics. Do not hand-refactor generated or pinned vendor code to satisfy style metrics.

## Interfaces and abstractions

Create abstractions at real boundaries, not everywhere.

Interfaces should be narrow and consumer-oriented. Add one when there is an architectural boundary, a meaningful test seam or genuinely interchangeable implementations. Do not create speculative factories, generic manager hierarchies or one-implementation interfaces solely to appear "clean".

Prefer:

- composition over inheritance;
- explicit ownership/lifetimes;
- immutable or single-owner state where practical;
- typed domain objects over long primitive parameter lists;
- pure functions for transformations/decisions;
- focused helpers local to the subsystem that owns the rule.

Avoid hidden side effects, duplicated state, temporal coupling and new global mutable state.

## ESP32-S3 hot-path/resource rules

For firmware/runtime changes:

- keep blocking I/O, e-ink waits, filesystem/network waits and long diagnostics outside the 1-second control hot path;
- avoid per-cycle heap churn and unbounded allocation in production control paths;
- prefer static/bounded storage where lifetime and maximum size are known;
- do not move large persistent buffers into scarce internal DRAM without measurement;
- use PSRAM intentionally for suitable bulk/non-latency-critical storage, while keeping DMA/capability requirements explicit;
- bounded queues/tasks must have a clear owner, capacity and overflow/failure behavior;
- a resource optimization must preserve ownership and safety semantics, not trade correctness for lower RAM.

When adding a recurring task, queue, cache or buffer, state its expected size/cadence and verify the affected diagnostics/build profile.

## Change discipline

Prefer the smallest production-quality change that fits the intended architecture.

When touching problematic legacy code:

1. do not make the structural problem worse;
2. preserve behavior with characterization/focused tests before risky refactoring;
3. extract a boundary when it is needed for the current change;
4. avoid broad cleanup unrelated to the task;
5. keep behavior change and large mechanical refactor separate when practical.

A passing test is not enough if the change introduces duplicated ownership, wrong dependency direction or a new God object.

## Architecture completion gate

Before considering a non-trivial implementation complete, review the diff and answer:

- Does every changed module still have one clear responsibility?
- Did a coordinator/composition/controller gain policy it should delegate?
- Did any central file grow when a focused module would be clearer?
- Is mutable state owned in one obvious place?
- Are dependency directions still correct?
- Did UI/display/transport/storage gain control authority accidentally?
- Is there duplicated policy or duplicated persisted/runtime state?
- Can core behavior be tested without booting the whole firmware?
- Did the change add unnecessary abstractions or framework leakage?
- Are RAM/CPU/task/queue costs bounded for firmware changes?
- Is the directory placement obvious to a new developer?

If the change introduces an architectural problem, fix that problem before declaring the work complete.

## Hardware boundaries

Authorized growbox serial device: `/dev/cu.usbserial-1130`.

Never open, probe, monitor, reset or flash `/dev/cu.usbserial-10`.
Do not use `/dev/cu.usbserial-1120` without explicit authorization.

For unattended qualification keep physical outputs disabled unless the operator explicitly changes that instruction.

## Frozen ownership/safety rules

- production Rule control remains authoritative;
- ML remains shadow/research-only;
- `OutputSupervisor` is the only normal owner of configured physical outputs;
- unavailable transport must not fabricate successful execution;
- one-way RF completion is transport evidence, not physical acknowledgement;
- raw RF remains restricted to explicit maintenance handling;
- lamp thermal trip remains `>= 28 C`;
- lamp recovery remains `<= 26 C` continuously for 10 minutes;
- e-ink/UI code is observer-side and must not mutate control or safety state;
- slow display work must stay outside the control hot path.

## Current display-port boundary

`vendor/litegraph_epd_port/` is read-only donor/reference material and must stay excluded from builds.

The integrated isolated Clay host component lives under `lib/growbox_clay_ui/`. It is C++20 internally and exposes growbox-owned Clay-free public types. The normal production application remains C++17. Current next display work is Phase 2 in `docs/DISPLAY_UI_PORT.md`: reproduce the existing Environment, Outputs, System and Diagnostics pages in the host simulator before firmware integration.

Do not replace the native SSD1680 backend, copy Arduino hardware ownership into production, or expose `Clay_*` types across the component boundary.

## Panel UI layout rule

Inside pot/actuator/target cards, related fields stay horizontal. Do not turn compact multi-field cards into vertical stacks or full-width mini-cells.

Canonical patterns:

- sensors: `.pot-card-sensors`;
- targets/cultivation: `.compact-row` + `.mini-cell`;
- actuators: horizontal `.field-stack`;
- pot cards: common `--pot-card-w` sizing.

### Układ strony

- left column: `card-stack` with Control, Sensors, Targets and Actuators;
- right column: live sensor/output state and `panel-actions`;
- previous state and other panel-action views open in one movable panel modal: `#modal-backdrop` -> `.panel-modal.modal--wide`.

### Antywzorzec

- vertical field stacks inside compact pot/cultivation cards;
- full-width mini-cells when related fields belong side by side;
- uneven two-column grids with large empty gaps;
- separate modal/tab layouts for views already owned by the single panel modal.

After changing `tools/panel/static/js/form.js` or `tools/panel/static/panel.css` run:

```bash
.venv/bin/python -m pytest tests/test_panel_layout.py -q
```

## Verification selection

Use the smallest relevant gate first, then widen only as needed.

- docs/config-only: `git diff --check` plus relevant schema/config guard if touched;
- formatting/lint: `make check-fast` when `.venv` is available;
- portable/runtime C++: focused regression first, then `make test-host` for affected runtime/display boundaries;
- Python/ML tooling: focused pytest first, then the relevant broader Python suite;
- panel UI: `make test-layout`, and `make test-panel` when behavior/API changed;
- Clay/display: component/simulator tests, display host suite, then ESP-IDF build before firmware integration claims;
- production firmware boundary/config changes: ESP-IDF build for the affected profile;
- hardware claims: exact-SHA Local Agent evidence on the authorized device.

Do not run expensive unrelated gates merely for ceremony, but do not skip the gate that covers the changed ownership/boundary.

## Completion rule

A commit proves publication, not execution. Report the exact commit plus relevant completed checks. Hardware claims require exact-SHA physical evidence.
