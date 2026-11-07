# BloomML — next sprint after IPO v2 CrowPanel build/NVS host qualification (2026-11-07)

## Start
- Repo `MichalMatu/bloomml`, main only for source/docs; `agent-control` only for Mac Local Agent. Fetch fresh HEAD and current daemon status before any action.
- Read `AGENTS.md`, `docs/README.md`, `docs/CURRENT_STATUS.md`, `docs/CONFIGURATION_FLOW.md`, `docs/ARCHITECTURE.md`, `docs/PROJECT_ROADMAP.md`, and [hardware qualification checkpoint](checkpoints/2026-11-07-bloomml-ipo-v2-hardware-qualification-gate.md).
- No Codex, Work, GitHub Actions, new branch, force push, or reopening completed B5/IPO host work.

## Closed — do not repeat
- CrowPanel real-input ESP-IDF build PASS on firmware source `1a1b3de8`, 80% of app partition free; Mac task `la-bloomml-ipo-v2-crowpanel-build-20261107-v1`, no flash.
- Production `IpoProfileNvsJournal.cpp` compiled with mock NVS API: power loss before/after marker flip, CRC/backup, rollback and output-namespace isolation; task `la-bloomml-ipo-v2-nvs-fault-matrix-20261107-v1` 1/1 PASS.
- LampSafety / TP357 stale-Rule / OutputRuntimeLifecycle focused tests 3/3 PASS.
- `make test-ipo` 3/3 PASS on `45f3527`; task `la-bloomml-ipo-v2-exact-source-final-gates-20261107-v2`. Fixed a Makefile CTest escaping regression.
- Prior B5 Vitest 9/9 remains accepted; do not rerun unnecessarily.

## Physical blocker
Mac Local Agent `/dev/cu.usbserial-120`: **ABSENT** in latest check. No physical board was opened, reset, flashed or tested. Thus this is **not hardware qualification**. Avoid `/dev/cu.usbserial-10`; do not use `/dev/cu.usbserial-1120` without explicit authorization.

## Next bounded sprint when authorized board is present
1. Confirm exact authorized port and physical isolation of output relays. Compile exact SHA with `GROWBOX_STAGE28_REAL_OUTPUTS_ENABLED=0`. Never interrupt a live growbox until isolation is verified.
2. Qualified real ESP-IDF NVS power-cut/CRC/backup/restart test; compare after reboot stored NVS vs active Mint, measure RAM/PSRAM/stack/watchdog.
3. TP357 and SCD41 missing/stale/invalid and lamp thermal trip/recovery matrix; verify output supervisor and emergency ventilation only under fake-locked outputs.
4. Independently review and implement supervisor-owned quiesce/emergency lock and permission scope; only then build authenticated local preview/activate/status/rollback endpoint in **existing** operator frontend. The historical demo `tools/panel` protocol is not a production security boundary.
5. Exact-SHA checkpoint; retain blocked status for any unavailable physical evidence.

## Non-negotiable
`ClosedIpoActivationGate` remains installed and there is **no physical IPO v2 activation**. Rule authoritative, ML/MPC shadow, TP357 safety >=28C trip / <=26C 10min recovery, `OutputSupervisor` sole configured output owner, no claimed physical RF acknowledgement, no auto-apply from NVS at boot.
