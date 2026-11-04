# BloomML — MPC research A–E: safety, predictive control and experiment gate

**2026-11-04 · research only, not a production controller.**

## Provenance / protected branches

- Repository: `MichalMatu/bloomml`, work branch `research/simulator-fidelity-kaggle-20261103`.
- Preflight research HEAD `42d2d0e4200f4c86064016acaaf099c97a779442`; first MPC implementation `1feaa6af3d2657faa3a3cbb54ee7cfcc24cc6d2a`; verified 60-min source `02ac7e244272db69e6649a37fd9c5ad272185723`; verified scenario matrix source `2ad2177943c4e3a50a4ad094bf939600f1ce14bf`.
- Production `main` observed unchanged at `51c90cd3290efde230f0c929e93aaf620e0de106`; no Growclip, firmware, Rule or OutputSupervisor edits. Mac Local Agent only cloned exact research source to a temp directory; no physical devices.
- Evidence: `agent-control:.agent/results/la-bloomml-mpc-r4-integration-pilot-20261104-v2.json`, `la-bloomml-mpc-r4-hourly-20261104-v2.json` and `la-bloomml-mpc-scenario-matrix-20261104-v1.json`. Source SHA must be compared against branch HEAD for each run; these are local agent results, not self-authenticating experimental manifests.

## Implemented research modules

| Owner | Responsibility / output |
| --- | --- |
| `tools/ml/physics/growbox_dynamics.py` (unchanged) | R4 0D air/wall/leaf/substrate, vapor/condensates, CO2, PPFD/DLI, estimated fan/coil and phase fluxes; **not calibrated** |
| `tools/ml/mpc/contracts.py` | Typed temperature, RH XOR VPD, CO2, PPFD/DLI targets, envelopes, bounded optimizer configuration and proposal status |
| `tools/ml/mpc/devices.py` | Virtual slew/dwell, capacities/faults, conflict interlocks, emergency stops and requested-versus-applied truth |
| `tools/ml/mpc/optimizer.py` | Seeded finite beam search through cloned R4 physics with energy/switch/target cost; timeout/fault/stale fallback |
| `tools/ml/mpc/lab.py` | Closed loop, threshold and stateful hysteresis baselines, per-tick forecasts/actuals/rejection ledger, MAE/RMSE and explicit tolerances |
| `tools/ml/mpc/cli.py` | Reproducible baseline/60-minute drift and setpoint-change CLI, source SHA, manifest and NDJSON |
| `tools/ml/mpc/scenario_matrix.py` | 11 adversarial category scenarios × configurable seeds × three independent controllers, SHA256 traces and per-run statuses |
| `tools/ml/mpc/shadow_contract.py` | **Passive only** proposal → hypothetical Rule verdict → hypothetical OutputSupervisor status, reject/timeout/no-ack; never executes hardware |

## Evidence: successful isolated scenario, not general MPC certification

- First **3-minute** 6 × 30 s pilot: 12/12 unit+R4 integration tests PASS on `1feaa6af...`, MPC T/RH/CO2 MAE `0.800 C / 2.504 pp / 60.117 ppm`; both simple baselines worse. Nameplate energy proxy and switch counts favor MPC; uncalibrated/short.
- **60-minute** 60 × 60 s disturbance+setpoint-change run on `02ac7e...`: **23/23 tests PASS**. Baselines and MPC used identical forcing and initial state.

| 60-minute metric | MPC | Threshold | Hysteresis |
| --- | ---: | ---: | ---: |
| T MAE (C) | **1.1168** | 6.3600 | 5.2187 |
| RH MAE (percentage points) | **5.7107** | 21.6017 | 24.1824 |
| CO2 MAE (ppm) | **11.3987** | 140.0813 | 137.7907 |
| Nameplate electrical *proxy* (Wh) | **223.60** | 380.16 | 379.59 |
| On/off transitions | **29** | 355 | 349 |
| Sampled safety envelope violations | **0** | 1 | 22 |
| Synthetic tracking gate | PASS | FAIL | FAIL |

The longest model-only directional/conservation test in the suite covers two hours at five-minute cadence; **it is not two hours of closed-loop MPC**. Closed-loop has so far been directly evidenced up to one hour in a single disturbance/setpoint scenario.

## Evidence: deliberate adversarial matrix / negative results

- Exact source `2ad2177943c4e3a50a4ad094bf939600f1ce14bf`, Mac Local Agent **33/33** targeted unit+R4+shadow tests PASS. Matrix: 11 scenarios × 2 seeds (`1729`, `1730`) × 3 policies = **66 synthetic 12-minute trials**, all without program errors. Aggregate matrix synthetic gate: **FAIL**, not PASS.
- MPC **3/22 PASS, 19/22 FAIL** under strict per-step tolerance/warm-up gate; threshold **0/22 PASS**, hysteresis **0/22 PASS**. The number of passes is not a statistical crop-control success rate; scenarios are intentionally adversarial and include infeasible/faulted conditions.
- MPC had **zero sampled hard envelope violations** in all 22 trials. Stale-sensor runs correctly generated six fallback ticks per seed and therefore FAIL, even though the physical state remained inside the safety envelope.
- **Negative comparison:** in both hot/wet starting states, simple threshold control had lower *temperature MAE* than MPC (MPC 2.195/2.081 C vs threshold 1.430/1.333 C); the full baseline still failed the combined gate. Do not claim MPC is universally superior.
- Other failure classes include cold-start convergence time, large setpoint steps, limited actuator capacity, impossible competing goals, heat-wave/RH mismatch and missing sensor evidence. They need attribution by warm-up period, reachable-set/actuator limits and model mismatch, not simply loosening the quality gate.
- `UNKNOWN` provenance cannot be promoted to `PASS`; actual violations/fallbacks are `FAIL` regardless of provenance. Scenario-level quality is distinct from the empirical fidelity status, which remains **UNKNOWN**.

## New safety hardening after the completed matrix (separate source verification required)

- Validate envelope/dwell/slew inputs; emit reasons for temperature/RH/CO2 stops, actuator maximum, slew, emergency stop, and conflict resolution.
- Emergency hard-stop must de-energize a *previously running virtual device* even during minimum-on hold. A hot chamber must prioritize cooling over heater when both were requested; high RH must prioritize removal over humidifying, and unsafe high CO2 must prioritize exhaust over dosing.
- This is **research-only** safety logic. It does not implement or override the frozen physical lamp trip/recovery and must never be represented as production qualification.

## Observability / exact quality gates

Records include source SHA, seed, step, external forcing and targets, measured-in-simulator state, projections and one-step model residuals, categorical uncalibrated uncertainty, proposed vs virtual applied actions, interlock and lifecycle rejection reasons, virtual faults, solver evaluation count, estimated power proxy, water inventory/additions/external exchange/residual, and per-policy metrics. `PASS / FAIL / BLOCKED / UNKNOWN` are separate research evidence states.

Independent tolerances: **temperature ±2 C**, **RH ±10 percentage points or VPD ±0.3 kPa**, **CO2 ±200 ppm**, **PPFD ±150 umol/m²/s**. A run fails if any fallback, any recorded safety violation, or more than 20% of samples are outside any per-target tolerance. The `>20%` rule penalizes initial warm-up and deliberate faults; quantify this separately from steady-state performance before declaring feasibility. DLI pacing is heuristic and has no day-boundary reset.

## Open limitations / technical debt

1. **Physical calibration P0:** 0D mixing/air distribution, plant physiological proxies, incomplete open-system energy/CO2 closure, fan/filter curve, real compressor COP, dehumidifier coil and water treatment, actuator delay/lag, sensor placement and confidence intervals all remain unverified. Synthetic water closure is not physics fidelity.
2. **Control robustness P0:** 19/22 MPC adversarial trials fail the synthetic quality gate, especially hot/wet initial states and infeasible/rapidly changing targets; identify reachable operating envelope, warm-up scorecard, fault state estimator, disturbance preview or robust MPC uncertainty sets. No evidence for day-long MPC performance.
3. **Observability P1:** nameplate `energy_proxy_wh` is not measured electricity; CO2 mass and open-system energy ledgers not independently closed; per-candidate search trajectories are summarized, not full optimizer tree; no actual sensor jitter/transport/network ACK evidence.
4. **Optimizer P1:** bounded search candidate set can miss optima; computation deadline checked between physics evaluations (not hard realtime interrupt); DLI reset/scheduling, safety-safe fallback per environment and joint target priority need explicit contracts.
5. **External-data limits P1:** real WUR and NUTRICONTROL greenhouse data are **other greenhouses**, not own growbox calibration. WUR mirror-vs-original 4TU equivalence unverified; no ground-truth power, delivered CO2 flow or PPFD in own box. No model trained/validated for production on these records.
6. **GPU / research D:** **NOT RUN** for MPC matrix. Old R4 dual-T4 evidence is about physics parity, not speedup or MPC performance. Large sweeps are on hold pending local diagnostic quality and a measured CPU/GPU benchmark under the same workload.
7. **Shadow E:** portable research contract and fake acceptance tests only. No firmware, ESP32-S3 resource test, Rule authority change, OutputSupervisor connection, real execution ACK, timeout or fallback demonstration.

## Missing data before any physical consideration

Measured geometry/surface capacities, heat/latent energy flux, exhaust vs speed/pressure and filter state, heater/cooler input electrical power and COP, humidifier/dehumidifier condensate and electricity, CO2 mass injection and mixing, canopy PPFD/DLI/calibrated spectrum and lamp electrical W, actual fan/circulation response, substrate moisture/plant area/transpiration, synchronized T/RH/CO2 sensors with age/bias/location, configured output request **and independently observed execution state**, failures/latencies, independent outdoor forcing and multi-day cultivation logs. Chronological independent-day calibration, uncertainty and model-vs-measurement benchmarks are required.

## Reproduction and advancement

```bash
PYTHONPATH=. python -m unittest -q tests.test_mpc_devices tests.test_mpc_optimizer_mock tests.test_mpc_baselines tests.test_mpc_device_responses tests.test_mpc_physics_integration tests.test_mpc_lab_observability tests.test_mpc_shadow_contract tests.test_mpc_scenario_catalog
PYTHONPATH=. python -m tools.ml.mpc.cli --hours 1 --step-s 60 --seed 1729 --output-dir build/mpc-hourly
PYTHONPATH=. python -m tools.ml.mpc.scenario_matrix --steps 12 --step-s 60 --seeds 1729 1730 --output-dir build/mpc-matrix
```

**Research implementation: RUNNING / VERIFIED ON THE LISTED EXACT SOURCE SHAs.** **Adversarial MPC qualification: FAIL (19/22).** **Empirical growbox model fidelity: UNKNOWN.** **Hardware/control deployment: BLOCKED.** The next research step is fault-aware hardening plus focused closed-loop failure attribution; do not scale to thousands of GPU cases merely to obscure known failures.
