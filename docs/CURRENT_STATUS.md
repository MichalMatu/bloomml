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

## IPO Runtime Bridge 3 (2026-11-07) — firmware v2 decoder and dormant staging source

Dodany w kodzie surowo walidujący `IpoProfileJsonDecode` (JSON v2 → bounded graph/targets → aktualne CrowPanel admission), nieaktywny `IpoProfileStagingCoordinator` (prepare / trwały zapis-only / weryfikacja), i `schedule_hash` diagnostyki istniejącego RAM Mint. Parser jest w źródłach real-input, ale brak jakiegokolwiek caller-a operacji NVS `commit` w produkcyjnym runtime; NIC nie zapisuje profilu, ani nie umożliwia jego zastosowania z panelu. Brak testów/kompilacji/flashowania: UNVERIFIED. `CONFIGURATION_FLOW.md` i checkpoint opisują granice.

## IPO Runtime Bridge 2 (2026-11-07) — durable profile source staged, NOT ACTIVE

Dodano `IpoProfileWire.h` (cały typowany graf i cele, bounded wersja+CRC) i `IpoProfileNvsJournal.h/.cpp` (oddzielna dwuslotowa domena NVS z odczytem zwrotnym i możliwością wykrycia dobrej poprzedniej kopii). Kod dołączono do listy źródeł real-input, ale nie jest wywoływany, niczego nie zapisuje/odczytuje w boot i nie zmienia aktywnego profilu. Brak weryfikacji wykonawczej lub sprzętowej; `active IPO v2` nadal NIE ISTNIEJE. Więcej: `CONFIGURATION_FLOW.md`.

## IPO Runtime Bridge 1 (2026-11-07) — code present, unqualified

W real-input firmware utworzono **jeden, typowany, walidowany właściciel profilu RAM** do celów Rule oraz harmonogramu lampy, z domyślnym bez zmian Mint. Do konsoli `status` dodano rozpoznawalny readback tego snapshotu (NVS=0/v2 applied=0). `lib/ipo_profile` ma teraz przenośny selektor świeżych sygnałów i rygorystyczne dopuszczenie referencyjnego grafu CrowPanel; bootstrap raportuje tylko informacyjną zgodność (apply=0). Nie ma jeszcze v2 JSON parsera na ESP, NVS profilu, włączonego fallbacku, dynamicznych wyjść ani live API. **Nie uruchamiano kompilacji/testów ani nie flashowano urządzenia.** Status tych zmian: UNVERIFIED. Patrz `CONFIGURATION_FLOW.md` i `checkpoints/2026-11-07-bloomml-ipo-runtime-bridge.md`.

## IPO Foundation v2 source sprint — 2026-11-07

The single browser homepage `/` now edits draft IPO v2: variable device instances, driver catalog, typed signals with primary/fallback, processes, logical outputs and many-to-one effect links. C++17 portable `lib/ipo_profile/src/IpoGraph.h` adds a bounded *unlinked* graph and symbolic mixer. Previous v1 is at `/lab/profile-v1`; research v5 stays `/lab/v5`. See `CONFIGURATION_FLOW.md`. Actual Rule/Safety/OutputSupervisor/RF433/NVS/e-ink unchanged: **no v2 firmware apply, dynamic I/O or live status**. Operator requested no tests or heavy builds; new code remains UNVERIFIED.

## Previous operator v1 draft (historical) — 2026-11-07

The first **offline** DeviceProfile v1 editor is now on the browser homepage (`/`): declarations for current TP357/SCD41/Xiaomi/DS3231 sources and the three fixed RF433 outputs, RH-or-VPD targets, day/night schedule and strict JSON import/export. The legacy research v5 editor is at `/lab/v5`; 3D is geometry-only. See [CONFIGURATION_FLOW.md](CONFIGURATION_FLOW.md) for the complete supported/unsupported matrix. **Firmware still uses the fixed Mint schedule; there is no profile apply, NVS storage or live browser status. No tests/builds were run for this sprint at the operator's direction.**

## Product-convergence baseline

The target is **one operator setup/status workflow**: real sensors/outputs -> desired targets/schedules/safety -> offline autonomous controller -> observed state and diagnostics. The validated browser draft is present, but the safe runtime apply/readback leg does not exist yet. The serial `tools/panel/` UI is a v4 developer/demo service; the browser `web/` now uses a separate v1 operator draft at `/`, retains v5 at `/lab/v5` and provides geometry-only 3D; production firmware is climate-v6 with fixed real-input composition. See `PRODUCT_DIRECTION.md` before extending any panel.

The former research branch's R4/Kaggle/MPC files and full commit history are now on `main`. Its 19/22 hard MPC scenario failures and missing physical fidelity prevent production promotion; see `docs/simulator/RESEARCH_STATUS.md`.

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
