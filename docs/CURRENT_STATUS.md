# Current controller status

Updated: **2026-11-07**
Repository: `MichalMatu/bloomml`
Canonical source branch: `main`

Status: **production feature/performance work remains paused pending the new-host baseline and hardware qualification**. A bounded 2026-11-07 product-direction/documentation and local demo-panel request-security repair was completed; it did not enable ML or change firmware control.

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

## Product-convergence baseline

The target is **one operator setup/status workflow**: real sensors/outputs -> desired targets/schedules/safety -> offline autonomous controller -> observed state and diagnostics. This does not exist end-to-end yet. The serial `tools/panel/` UI is a v4 developer/demo service; the browser `web/` page exports v5 research JSON and separately previews 3D geometry; production firmware is climate-v6 with fixed real-input composition. See `PRODUCT_DIRECTION.md` before extending any panel.

The separate `research/simulator-fidelity-kaggle-20261103` branch has experimental R4/Kaggle/MPC work (110 commits ahead of the common base as observed). Its 19/22 challenging MPC scenario failures and missing physical fidelity evidence prevent a production promotion. Research is not merged into `main`; see branch-local `docs/simulator/RESEARCH_STATUS.md`.

This scoped repair's evidence and remaining full-audit gates live in `AUDIT_STATUS.md`; the next step is `NEXT_CHAT.md`. The current main code/firmware has NOT been completely requalified on the latest exact SHA.

## MPC / Kaggle research now in main (2026-11-07)

The R4 numerical simulator, Kaggle runners/notebooks, external greenhouse dataset tooling, and experimental MPC have been merged into `main` while staying exclusively under `tools/ml/`, `tests/`, `notebooks/` and `docs/simulator/`. Code inclusion is **not** an approval for physical actuation. Latest research record: [research status](simulator/RESEARCH_STATUS.md), [MPC roadmap](simulator/MPC_ROADMAP.md) and [2026-11-04 exact-SHA checkpoint](checkpoints/2026-11-04-bloomml-mpc-resume-complete.md).

Historical MPC challenge matrix: **3/22 PASS, 19/22 FAIL**; a 60-minute representative MPC episode was PASS, but physical model calibration/own growbox logs and active MPC hardware approval remain absent. Separate research branch history is preserved as a merge parent. New work goes directly on `main`, keeping the research / runtime ownership boundary.

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
