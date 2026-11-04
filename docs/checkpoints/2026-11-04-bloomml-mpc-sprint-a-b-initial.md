# BloomML — MPC Sprint A/B research checkpoint (2026-11-04)

## Preflight
- Research branch observed HEAD before changes: `42d2d0e4200f4c86064016acaaf099c97a779442`.
- Production `main` observed: `51c90cd3290efde230f0c929e93aaf620e0de106`; no edits.
- `agent-control:.agent/status/daemon.json` read: `idle`, `current_task_id=null`, timestamp `2026-11-04T01:07:25.566973+00:00`. Snapshot, not a lease.
- AGENTS.md and mandatory simulator/checkpoint material read. No Local Agent tasks, GPU jobs, physical hardware or Actions started.

## Implementation
- Added separate research-only MPC contracts, fail-closed policy and virtual device execution, beam-search lookahead and offline baseline laboratory.
- Reused R4 model rather than duplicating physical equations; no changes to R4 physics, firmware or main.
- Explicit 2 C / RH 10 pp / VPD 0.3 kPa / CO2 200 ppm / PPFD 150 umol/m2/s tolerances, no global 1% claim.
- Added scenario CLI that uses the same forcing and initial state for MPC / threshold / hysteresis.

## Evidence and open gates
- Sandbox unit tests: `python -m unittest -q tests/test_mpc_devices.py tests/test_mpc_optimizer_mock.py` — 8 PASS (pure virtual-device and **fake-physics** optimizer tests).
- Syntax compile: `python -m compileall -q tools/ml/mpc` — PASS.
- Full original R4 physics integration suite **UNKNOWN / NOT EXECUTED in the sandbox**, because this environment has no checkout or network access to fetch private GitHub sources. A dedicated real-physics test file is committed but is **not** evidence that it ran.
- Multi-hour/multi-day E2E, physical accuracy/empirical calibration, Kaggle 2x T4, GPU parity, and CPU/GPU speed claims: **UNKNOWN / NOT EXECUTED**.
- Therefore next-phase readiness: **BLOCKED pending exact-SHA R4 integration execution and multi-seed baseline results**. This checkpoint is not a complete autonomous qualification.

Known risks: uncalibrated 0D physics, non-closed open-system enthalpy ledger, actuator delay approximated by slew/dwell, DLI day-reset not implemented, approximate energy accounting, no per-channel sensor transport lag or execution ACK, simple beam-search candidate coverage (not a global optimum), computation budget can trigger safe fallback. No transfer to hardware permitted.

## Reproduction
`PYTHONPATH=. python -m unittest -q tests.test_mpc_devices tests.test_mpc_optimizer_mock tests.test_mpc_physics_integration`

`PYTHONPATH=. python -m tools.ml.mpc.cli --hours 0.5 --seed 1729 --output-dir build/mpc-pilot`

The SHA of the **actual code commit must be resolved from the published research branch**; this checkpoint cannot predeclare its own commit SHA.
