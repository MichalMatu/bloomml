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
