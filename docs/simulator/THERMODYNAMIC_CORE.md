# BloomML: coupled thermo-hygro-plant core (research branch)

**Owner:** `tools/ml/physics/growbox_dynamics.py` (physical state/fluxes),
`growbox_psychrometrics.py` (SI psychrometric functions),
`growbox_measurements.py` (instrument registry / provenance boundary).
**Production climate-v6:** unchanged; Rule + OutputSupervisor remain authoritative.
This is a **new research reference model**, not a verified digital twin of the physical growbox.
The existing `climate_simulator.py` continues to serve historical climate-v6 training
until a versioned adapter and physical qualification are accepted.

## Physical boundaries

- Well-mixed growbox air volume and independent dynamic temperatures for air,
  walls, canopy/leaf and substrate. No unearned claim of 3D CFD accuracy.
- SI-based balances: sensible heat [W], ventilation [m3/s], vapor and liquid water
  [kg], CO2 mass injection [g/h], and plant carbon flux [umol/m2/s].
- Moist-air density uses total atmospheric pressure. A pressure-difference sensor
  can be recorded, but pressure-drop -> airflow requires a **measured fan curve**;
  avoid inventing leakage from measured pressure.
- RH is derived from **vapor mass and air temperature**, never advanced as
  independent conserved matter. Vapor saturation, air/leaf VPD, dewpoint, leaf
  and wall condensation/re-evaporation, coil dehumidification, evaporative
  humidifier cooling and saturated-air fog are represented.
- Water bookkeeping explicitly covers air, soil, wall/leaf/fog condensate,
  cooler/dehumidifier condensate, drainage, external exchange and added water.
  `water_balance_residual_kg()` tests conservation.
- Thermal exchange: wall-to-room/air, leaf-to-air, substrate-to-air, outside
  ventilation, heater, cooler, lighting, sunlight, fan/equipment heat, and
  latent heat of transpiration/condensation.
- Light energy: **measured electrical W** or nameplate×dimming; heat partitioned
  among air/leaf/walls exactly once. **Measured canopy PPFD** controls plant
  responses and DLI but is **not** added as extra heat. Optional sunlight heats
  outer walls even with an opaque enclosure; window area allows transmission.
- Plant proxies: light-saturated CO2 uptake, night respiration, light/VPD/water
  dependent transpiration, leaf temperature and bare-substrate evaporation.
  Empirical coefficients must be fit against local measurements.

## Measurement strategy

`growbox_measurements.CHANNELS` enumerates over **50 instrument channels** with
units and plausible ranges: T/RH/CO2 and gradients, pressure/air speed/airflow,
leaf/wall/soil temperatures, PPFD/PAR/DLI inputs, power meters, spectrum,
substrate water, mass/EC/pH, water flow, reservoir, condensate and CO2 flow.
Dewpoint, air/leaf VPD and DLI are **derived**, not standalone fabricated sensor
truth. Lux is **not PPFD** without spectral calibration.

Each measurement carries sensor ID, observed time, provenance type and validity.
Only fresh readings from real instruments enter boundary forcing; all other
readings are tagged missing/stale/synthetic. Chamber/leaf observations remain
held-out for comparison and must not be injected into a predictive simulation.
The registry documents potential measurement inputs; it does **not** mean the
current ESP32/SD logger records them all. Adding hardware/logging requires a
separate explicit design and authorization.

## Equations / assumptions

- Ventilation: `Q = exhaust_m3/h / 3600 + infiltration_ACH * V/3600`.
  `dm_vapor/dt = Q*(rho_v,out-rho_v,in) + sources - sinks`.
  `dCO2_ppm/dt = Q/V*(outside-inside) + injection + plant_flux`.
- Moist-air heat: `dT_air/dt = [Q_heater + Q_lamp,air - Q_cool + Q_vent
  + UA_wall*(T_wall-T_air) + UA_leaf*(T_leaf-T_air)
  + UA_sub*(T_sub-T_air) + latent + equipment]/(rho_air*c_p*V*m)`.
- Surface heat: `dT_surface/dt = (radiant + convection + conduction
  + condensed_water*latent_heat)/(C_surface)`.
- Wall condensation: `dm/dt = h_mass * A_wall * [rho_v,in - rho_sat(T_wall)]`,
  bounded by available water and stored condensate (can reverse sign).
- Air VPD = `[e_sat(T_air)-e_vapor]/1000`, leaf VPD =
  `[e_sat(T_leaf)-e_vapor]/1000`; the latter may be negative during dew.
- DLI += `PPFD * dt * 1e-6` (mol/m2), **requires actual PPFD history**.
- Multi-rate integration: bounded explicit substeps constrained by exchange and
  heat-transfer time constants; reject excessive/unphysical simulation steps.

## Limits and qualification requirements

This is not a first-principles CFD/crop physiology solver. Remaining refinements:
local flow distribution / pressure fan curve, spectral absorption and view
factors, wall thermal-layer detail and radiative transfer, phase change near
freezing, precise humidifier/coil psychrometrics and enthalpy, mechanistic leaf
stomatal resistance, soil water potential/field capacity/hysteresis and sensor
lag, bias and spatial placement. Atmospheric pressure is modeled as an outside
boundary, not a sealed-chamber pressure ODE. The constants supplied in code are
**illustrative priors** and not fitted truths.

### Physical acceptance (before large Kaggle sweeps)

1. Unit/limiting cases including zero inputs, no vent/plant, step-size
   convergence, physical bounds, lamp-energy bookkeeping, mass/energy closure.
2. Import real synchronized sensor histories. Preserve valid/fresh flags,
   calibration and provenance. Avoid using controller **requests** as physical
   proof of actuator execution. Record airflow/heat and operating state.
3. Estimate uncertain coefficients on chronological training runs;
   test against unseen days, devices, light cycles, moisture and failure cases.
   Report MAE/RMSE/bias in T, RH, CO2, leaf T, PPFD, and multi-horizon rollouts,
   including the simple persistence baseline and confidence intervals.
4. Add GPU-batched backend **with numerical parity** against this reference
   implementation, then run controlled scenario sweeps. Two T4 16GB GPUs are
   independent; distribute trajectories between devices, not an imaginary
   unified 32GB accelerator. Verify available devices and quotas on Kaggle.
5. Train small student/predictor from validated trajectories. Compare against
   Rule and persistence; keep ML shadow-only and never bypass physical safety.

## Simple research example (no hardware)

```python
from tools.ml.physics.growbox_dynamics import (
    GrowboxThermodynamics, GrowboxPhysics, GrowboxState, GrowboxForcing
)
physics = GrowboxPhysics()
model = GrowboxThermodynamics(
    physics, GrowboxState.initial(volume_m3=physics.geometry.air_volume_m3)
)
forcing = GrowboxForcing(lamp_dimmer=1.0, exhaust_fan=0.2)
for _ in range(360):
    model.step(forcing, 10)
observation = model.observe(forcing)
print(observation, model.water_balance_residual_kg())
```

Run tests: `PYTHONPATH=. python -m pytest -q tests/test_growbox_thermodynamics.py tests/test_growbox_measurements.py`.

## Reproducible CPU trajectories / eventual Kaggle GPU parity

The reference workload makes fixed-seed correlated random growbox geometries,
lighting and device scenarios. Each NDJSON row has an explicit
`origin: synthetic_uncalibrated` and input seed/episode/step, which **cannot**
be confused with a real sensor measurement or a qualified training dataset.

```bash
python -m tools.ml.physics.run_thermodynamic_sweep \
  --scenarios 40 --steps 720 --seed 1729 \
  --summary build/thermodynamic-golden.json \
  --output build/thermodynamic-golden.ndjson
```

The result records a SHA-256 digest of canonical trajectory lines, as well as
water-mass residuals and final climate summaries. A future batched Torch/CUDA
backend for Kaggle **must** compare to this golden CPU solver across limiting
cases and step sizes before scaling to two T4 devices. Heavy sweeps without
real calibration would only amplify uncertainty, not produce accurate plant
control. Neither Kaggle nor a GPU is needed for the reference checks.
