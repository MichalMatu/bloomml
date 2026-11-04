# BloomML research-only MPC (uncalibrated)

Owner: `tools/ml/mpc`. There is no firmware, network, GPIO, RF, relay or production integration. Model: `tools/ml/physics/growbox_dynamics.py`, unmodified. Controller calls its exact `GrowboxThermodynamics.step()`; repeated clones do not mutate the running plant.

Modules:
- `contracts.py`: typed setpoints (T, RH **or** VPD, CO2, PPFD, optional DLI), quality/evidence contract and bounded search config.
- `devices.py`: functional virtual actuator state, slew/dwell/failure/interlocks and stateless hard safety envelope.
- `optimizer.py`: seeded bounded beam search over actual reference physics, soft normalized multiobjective loss, safe fallback on stale readings, invalid physics or computation timeout.
- `lab.py`: isolated simulated plant, virtual execution feedback, setpoint/forcing schedules, threshold and hysteresis comparators, time-series and metrics.
- `cli.py`: finite offline three-way comparison, JSON manifest and NDJSON trajectories.

Example from repository root:

```bash
PYTHONPATH=. python -m unittest -q tests.test_mpc_devices tests.test_mpc_optimizer_mock tests.test_mpc_physics_integration
PYTHONPATH=. python -m tools.ml.mpc.cli --hours 0.5 --seed 1729 --output-dir build/mpc-pilot
```

Engineering tolerances: T 2 C, RH 10 percentage points **or** VPD 0.3 kPa, CO2 200 ppm, PPFD 150 umol/m2/s. A result is `PASS` only with valid 40-character source SHA, no safety violations/fallbacks and at most 20% of steps outside per-variable tolerances. `UNKNOWN` is not PASS. Unreachable setpoints are failures, not masked. `FAIL` includes any sampled unsafe state or any fallback. Baselines use identical plant, forcing and setpoint schedule; neither includes PID yet. Sensor dropout is an age gate, not an emulated sensor lag observer; actual sensor delay/missing-value replay is future work.

All device ratings, heat/lamp partition, CO2 and humidity coefficients, compressor COP proxies, capacity bounds, delays and slew/dwell defaults are research assumptions **not measured data**. `energy_proxy_wh` and `energy_weight` are nameplate-based heuristic penalties (not electricity-meter measurements), `water_residual_kg` is conserved as in R4, not a physical validation. Open-system energy enthalpy closure and independent CO2 flux ledger are NOT solved by this sprint. Air/leaf/substrate thermal inertia comes from R4; equipment actuator delay here is modeled as slew and dwell only. DLI pacing is heuristic, and cumulative DLI is not yet reset at a photoperiod/day boundary.

Safety in this offline module is not a certified fail-safe. Fallback only proposes ventilation/circulation to the **virtual** devices; an actual failure of ventilation is independently recorded. Commands always carry separate requested vs applied values, never assume physical execution acknowledgment.

Gates before broad Kaggle: full R4 integration tests under a checked-out exact SHA; replay closed-loop with CPU time/quality budget; multi-seed baseline table and error reporting; inspect any hard safety violations. Only after that evaluate whether two T4s help physics/scenario evaluation. Do not invent GPU acceleration or real fidelity.

Future shadow boundary (research contract only):
`MPC proposal + source SHA + observed_at + sensor validity + confidence -> Rule verdict -> OutputSupervisor execution intent -> actual acceptance/rejection/timeout/acknowledgement -> fallback`. No production code has been altered.

## Verification after first published research slice (2026-11-04)

The first exact-SHA Mac Local Agent integration at `1feaa6af3d2657faa3a3cbb54ee7cfcc24cc6d2a`
completed **12/12** targeted unit + original R4 integration tests (`la-bloomml-mpc-r4-integration-pilot-20261104-v2`, exit 0).
The initial 6 × 30s synthetic disturbance/setpoint episode gave MPC T/RH/CO2 MAE
0.800 C / 2.504 pp / 60.117 ppm, versus 2.809 C / 13.707 pp / 115.680 ppm for the
memoryless threshold baseline; baseline and deadband control happened to agree in that
short episode. MPC energy **nameplate proxy** was 9.587 Wh vs 18.186 Wh (not metered).
There were zero sampled envelope violations for all three policies. The pilot is not
statistically representative or physical fidelity evidence.

The following incremental changes require their own exact-SHA integration run:
- true stateful hysteresis (Schmitt trigger) instead of a memoryless deadband;
- per-step projected-versus-actual residual and explicit virtual device execution
  status (`simulated_applied` / `faulted_simulation`), never a physical ACK;
- independent R4 directional-response and two-hour conservation tests;
- safety cost uses the supplied custom envelope, not hidden hardcoded constants;
- an observed failure returns `FAIL` even if SHA provenance is missing; merely
  incomplete otherwise valid provenance returns `UNKNOWN`.

### Observation limits

This is an *open-loop prediction inside a closed-loop controller*: the optimizer
holds today's known external forcing fixed for its short horizon; the lab may
change the forcing at the next control tick. The next-step residual quantifies
that discrepancy but **does not** establish confidence intervals, calibrated
model uncertainty, real sensor freshness or observed physical execution.
A simulated actuator is deterministic and always reports its own application.
Sensor dropout currently gates on sample age, not lagged state reconstruction.
Time limits are checked between full reference-physics evaluations, so are
best-effort wall-clock budgets, not hard real-time preemption.
The threshold baseline is memoryless; the hysteresis baseline now remembers
which virtual devices are on and uses a Schmitt-trigger deadband.

## Completed 60-minute original-R4 CPU closed loop

The **exact-SHA** Mac evidence `agent-control:.agent/results/la-bloomml-mpc-r4-hourly-20261104-v2.json`
for source `02ac7e244272db69e6649a37fd9c5ad272185723` reports **23/23** tests PASS;
first test job v1 had one incorrect fixture expecting UNKNOWN despite an actual tolerance
failure (22/23 pass), and was corrected without weakening the gate.
The 60 × 60 s test used an ambient-temperature step and target change:

| Metric | MPC | Threshold | Stateful hysteresis |
| --- | ---: | ---: | ---: |
| T MAE (C) | 1.1168 | 6.3600 | 5.2187 |
| RH MAE (pp) | 5.7107 | 21.6017 | 24.1824 |
| CO2 MAE (ppm) | 11.3987 | 140.0813 | 137.7907 |
| Nameplate energy proxy (Wh) | 223.60 | 380.16 | 379.59 |
| Switches | 29 | 355 | 349 |
| Sampled envelope violations | 0 | 1 | 22 |
| Synthetic quality gate | PASS | FAIL | FAIL |

Max one-step temperature prediction discrepancy for this specific run was 0.01383 C;
that is **in-model residual**, not prediction error against real measurements.
Water residuals were below ~1.1e-14 kg on all policies. Both baselines can violate
virtual envelopes under disturbance; do not conceal those failures or extrapolate
this single simulation to other devices, seasons or real plants.

## Scenario and shadow research extensions

Use `python -m tools.ml.mpc.scenario_matrix --steps 12 --step-s 60 --seeds 1729 --output-dir build/mpc-matrix`
to evaluate **11 diagnostic scenarios**, including cold/hot start, ambient cold/heat,
RH/CO2 shifts, setpoint change, actuator fault, stale sensor, limited device
capacity and competing targets. All strategies share the same initial state,
forcing and virtual faults for each seed. The output includes a per-scenario,
per-policy quality verdict and trajectory hash; negative trials remain FAIL.

`tools/ml/mpc/shadow_contract.py` and its tests are *passive research DTOs* for
`MPC proposal -> hypothetical Rule verdict -> hypothetical Supervisor feedback`.
A `SIMULATED_ACK` cannot imply physical acknowledgement; mismatched IDs, stale data,
absent execution evidence, rejection and timeout are distinguishable outcomes.
No adapter, transport, firmware write or output execution is implemented.

These extension files must be separately committed and verified on their exact
source SHA before describing their scenario-run evidence as COMPLETE.

## Adversarial matrix outcome and next gate

At source `2ad2177943c4e3a50a4ad094bf939600f1ce14bf`, the Mac Local Agent
`la-bloomml-mpc-scenario-matrix-20261104-v1` ran **33/33 targeted tests PASS**
and a complete **11 × 2 × 3 = 66-trial** diagnostic CPU matrix. MPC tracking
quality gate: **3/22 PASS, 19/22 FAIL**; threshold/hysteresis both 0/22 PASS.
In hot/wet starts, the threshold controller actually beat MPC on *temperature
MAE* despite failing the overall multi-target gate. All MPC trials had zero
sampled hard envelope violations; stale sensor cases correctly fell back.
This is a reproducible negative result, not grounds for running an unqualified
Kaggle scale-out. Detailed evidence and shortcomings are in
[the MPC A–E checkpoint](../checkpoints/2026-11-04-bloomml-mpc-sprint-a-e-research-gate.md).
