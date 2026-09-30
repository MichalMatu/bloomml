# Current controller status

Updated: **2026-09-30**  
Repository: `MichalMatu/bloomml`  
Canonical source branch: `main`

Status: **feature and performance work is paused until the MacBook M1 Pro / 32 GB host is available.** No implementation slice or benchmark tuning is intentionally left in progress.

## Current product state

- Deterministic Rule control is production authority.
- ML remains shadow/research-only and must not become control authority by convenience.
- `OutputSupervisor` remains the normal owner of configured physical outputs.
- Unavailable transport must not be reported as successful execution.
- E-ink/UI remains observer-side and must not mutate control, output or safety state.
- Slow display, filesystem and network work stays outside the 1-second control hot path.
- Lamp thermal trip remains `>= 28 C`.
- Lamp recovery remains `<= 26 C` continuously for 10 minutes.

The current architecture split remains intentional: `ClimatePolicy.*` owns stateless Rule generation/arbitration/safety policy, while `ClimateRuntimeController.*` owns stateful trend/effective-action estimation, ML orchestration and execution reconciliation.

## Display / hardware baseline

The production CrowPanel display path is integrated and stable at the software boundary:

- native ESP-IDF SSD1680 backend remains the sole hardware/framebuffer owner;
- exactly one 296x128 / 4736-byte framebuffer is retained;
- Clay rendering is isolated behind growbox-owned C++17-compatible semantic types;
- display work remains asynchronous;
- reduced partial refresh is integrated;
- Home/Back/Previous/Next/OK input and the read-only four-page chooser are integrated without taking control/output ownership.

Existing exact-SHA hardware evidence showed healthy display refresh, stable heap/PSRAM and no panic/brownout/watchdog failure with physical outputs fake-locked. Completed phase-by-phase evidence belongs in `docs/HISTORY.md`, `docs/CHANGELOG.md` and Git history rather than this status file.

Remaining physical qualification is intentionally explicit rather than disguised as software debt:

- SCD41 behavior must be re-qualified with the sensor physically present;
- manual key navigation and visual/ghosting checks require operator interaction;
- destructive or real-output hardware qualification must follow the current `AGENTS.md` hardware rules.

## Performance state

The completed local build/test acceleration pass is merged. Current defaults remain:

- ESP-IDF ccache enabled by default;
- `HOST_BUILD_JOBS=4`.

No CPU-vs-Apple-GPU ML A/B was run before the pause. The current ML dependency set does not pin `tensorflow-metal`, so GPU availability must be probed on the new host rather than assumed.

The canonical benchmark history and new-Mac comparison procedure are in `docs/PERFORMANCE_HANDOFF.md`. Do not change dependencies, worker counts or device placement before recording the fixed baseline.

## Resume order

When the M1 Pro / 32 GB machine is available:

1. fetch fresh `main` and fresh `agent-control` daemon state;
2. read `AGENTS.md`, `docs/README.md`, this file, `docs/ARCHITECTURE.md`, `docs/PROJECT_ROADMAP.md` and `docs/PERFORMANCE_HANDOFF.md`;
3. rerun the fixed historical build/test baseline on the new Mac for an apples-to-apples host comparison;
4. establish a fresh current-`main` CPU/RAM/cache baseline;
5. probe the installed TensorFlow device/plugin state before deciding whether a CPU/GPU A/B is possible;
6. only after those baselines consider higher build parallelism, Metal dependencies, or another product slice;
7. re-open physical SCD41/button qualification only when the required hardware/operator interaction is available.

Historical Stage/Phase handoffs, closed audit narratives and benchmark task ids are not active state. Recover them from Git history when evidence is needed instead of expanding this file again.
