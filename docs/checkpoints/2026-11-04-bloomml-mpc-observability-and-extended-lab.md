# BloomML MPC — incremental research checkpoint (2026-11-04)

## Source and completed evidence

- Baseline repository HEAD `42d2d0e4200f4c86064016acaaf099c97a779442`; first MPC source commit `1feaa6af3d2657faa3a3cbb54ee7cfcc24cc6d2a` on `research/simulator-fidelity-kaggle-20261103` only.
- Mac Local Agent terminal result `agent-control:.agent/results/la-bloomml-mpc-r4-integration-pilot-20261104-v2.json`, `done`, exit 0: **12/12** targeted tests on exact source commit, plus three-minute, six-tick original-R4 closed-loop pilot.
- In that *single synthetic pilot*, MPC MAE T `0.79973 C`, RH `2.50424 pp`, CO2 `60.1171 ppm`, versus threshold and deadband T `2.80873 C`, RH `13.70716 pp`, CO2 `115.6801 ppm`. MPC virtual-device switch count 5 vs 15; nameplate power proxies 9.5873 Wh vs 18.1862 Wh. Zero sampled hard envelope violations. The result is not multi-hour performance, crop efficacy, measured electrical energy, calibrated physics or real-world controller qualification.
- Previous Mac Local Agent job v1 had shell diagnostic typo (unquoted `|`, exit 127) before tests. It must NOT be counted as an MPC failure.

## New architecture and contracts

- No firmware, production Rule, OutputSupervisor, `main`, Growclip, GPIO, relays or hardware changes.
- Physical solver stays in `tools/ml/physics`; research `tools/ml/mpc` owns proposal, virtual device lifecycle, planning and experiment orchestration independently.
- `FAIL` if safety violations, recorded fallback or excessive samples outside variable-specific tolerances; `UNKNOWN` if no trustworthy SHA and otherwise no known failure. Model fidelity remains `UNKNOWN` even if synthetic quality gate `PASS`.
- Per-cycle NDJSON: model targets/state, proposed/applied/rejected commands and reasons, predictions and first-step residual, optimizer cost/evaluation count, provenance/seed, virtual fault status, water-mass ledger and nameplate energy proxy. No unsupported physical execution acknowledgment.

## Pending extended validation

The source amendments in this checkpoint require exact-SHA tests and multi-hour scenario evidence **before** their PASS status can be claimed. Add failure injections, capacity limitations, disturbances and multi-seed baseline comparison. A 2× T4 Kaggle study is BLOCKED until those local gates and compute benefit design are measured.

## Research boundaries

- 0D thermo-hygro-CO2 physics, uncalibrated thermal inertia, fan curve, humidifier and CO2 injection parameters. Open-system energy/CO2 closure and physical sensor/actuator delays lack empirical qualification.
- Electricity measured `UNKNOWN` (proxy only); DLI reset/daylight scheduling `UNKNOWN`; model uncertainty is a categorical warning, not a calibrated interval; no hardware Rule/OutputSupervisor integration.
- No threshold for total "1% error"; T 2 C, RH 10 pp or VPD 0.3 kPa, CO2 200 ppm, PPFD 150 umol/m2/s assessed separately.

**Status for hardware: BLOCKED. Status for next offline research phase: CONDITIONAL on an exact-SHA extended E2E run and independent regression evidence.**
