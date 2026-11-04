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
