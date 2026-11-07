"""Scalar-oracle parity and numerical safety of vectorized research physics.

CPU PyTorch tests do not claim any actual CUDA execution. The Kaggle validation
must run on both real T4s before GPU results are accepted.
"""
from __future__ import annotations

from dataclasses import fields, replace
import math
import random

import pytest

torch = pytest.importorskip('torch')

from tools.ml.physics.growbox_dynamics import (
    GrowboxPhysics, GrowboxForcing, GrowboxState, GrowboxThermodynamics,
)
from tools.ml.physics.growbox_tensor import simulate_batch


def _check_oracle(parameters, states, forcings, *, dt=10, ticks=7, dtype=torch.float64,
                  absolute=3e-9, relative=3e-9):
    reference = []
    reference_residuals = []
    for p, state, forcing in zip(parameters, states, forcings):
        model = GrowboxThermodynamics(p, state)
        for _ in range(ticks):
            model.step(forcing, dt)
        reference.append(model.state)
        reference_residuals.append(model.water_balance_residual_kg())
    batch = simulate_batch(parameters, states, forcings, dt_s=dt, ticks=ticks,
                           device='cpu', dtype=dtype)
    assert batch.scenarios == len(states)
    assert batch.ticks == ticks
    for i, (gold, actual) in enumerate(zip(reference, batch.states)):
        for f in fields(GrowboxState):
            x, y = getattr(gold, f.name), getattr(actual, f.name)
            assert math.isclose(x, y, rel_tol=relative, abs_tol=absolute), (i, f.name, x, y)
        assert abs(batch.water_residuals_kg[i] - reference_residuals[i]) < max(absolute, 1e-12)
    return batch


def test_reference_default_trajectory_and_state_immutable():
    p = GrowboxPhysics()
    s = GrowboxState.initial()
    f = GrowboxForcing(lamp_dimmer=0.6, exhaust_fan=0.3, circulation_fan=0.9)
    result = _check_oracle([p]*3, [s]*3, [f]*3, ticks=15)
    assert s.elapsed_s == 0
    assert result.states[0].elapsed_s == pytest.approx(150)
    assert max(map(abs, result.water_residuals_kg)) < 1e-11


def test_heterogeneous_scenarios_pressure_fan_lamp_and_condensation():
    rng = random.Random(1703)
    params, states, forcings = [], [], []
    for i in range(24):
        p = GrowboxPhysics()
        g = replace(p.geometry, air_volume_m3=rng.uniform(.20, 2.0),
                    leaf_air_conductance_w_k=rng.uniform(1, 23),
                    wall_vapor_transfer_m_s=rng.uniform(0, .0004))
        b = replace(p.biology, maximum_transpiration_g_h=rng.uniform(0, 140))
        e = replace(p.equipment, exhaust_max_m3_h=rng.uniform(0, 190))
        l = replace(p.lighting, rated_power_w=rng.uniform(0, 260))
        params.append(replace(p, geometry=g, equipment=e, biology=b, lighting=l))
        t = rng.uniform(18, 30)
        states.append(GrowboxState.initial(air_temperature_c=t, rh_pct=rng.uniform(15,99),
                      co2_ppm=rng.uniform(350, 1450), volume_m3=g.air_volume_m3,
                      wall_temperature_c=t-rng.uniform(0,7), leaf_temperature_c=t-rng.uniform(0, 4),
                      substrate_temperature_c=t-rng.uniform(0,4),
                      substrate_water_kg=rng.uniform(.15,2.5)))
        forcings.append(GrowboxForcing(
            outside_temperature_c=rng.uniform(12,30), outside_relative_humidity_pct=rng.uniform(20,95),
            atmospheric_pressure_pa=rng.uniform(90000,105000), outside_co2_ppm=rng.uniform(350,750),
            lamp_dimmer=rng.random(), lamp_measured_electrical_w=(rng.uniform(0,200) if i%3==0 else None),
            canopy_measured_ppfd_umol_m2_s=(rng.uniform(0,750) if i%4==0 else None),
            measured_exhaust_m3_h=(rng.uniform(0,100) if i%5==0 else None),
            measured_air_speed_m_s=(rng.uniform(0,1.3) if i%5==1 else None),
            solar_irradiance_w_m2=rng.uniform(0,700), heater=rng.random(), cooler=rng.random(),
            exhaust_fan=rng.random(), circulation_fan=rng.random(), humidifier=rng.random(),
            dehumidifier=rng.random(), co2_doser=rng.random(), irrigation_kg_s=rng.uniform(0,1e-5)))
    result=_check_oracle(params, states, forcings, dt=7.7, ticks=9)
    assert len(set(result.substeps_per_tick)) > 1, 'heterogeneous adaptive substep masking exercised'


def test_fog_dew_and_water_mass_with_hot_and_cold_surfaces():
    p=GrowboxPhysics()
    states=[GrowboxState.initial(air_temperature_c=28, rh_pct=100, volume_m3=.8,
              leaf_temperature_c=11, wall_temperature_c=9),
            GrowboxState.initial(air_temperature_c=26, rh_pct=99, volume_m3=.8,
              leaf_temperature_c=26, wall_temperature_c=26)]
    forcings=[GrowboxForcing(cooler=1, dehumidifier=.5, humidifier=1),
              GrowboxForcing(heater=.7, exhaust_fan=1, circulation_fan=1, irrigation_kg_s=5e-6)]
    out=_check_oracle([p,p],states,forcings,dt=2,ticks=2)
    assert out.states[0].wall_condensate_kg > 0
    assert out.states[0].leaf_condensate_kg > 0
    assert max(map(abs,out.water_residuals_kg)) < 1e-11


def test_float32_drift_is_explicitly_bounded_not_exact_parity():
    p=GrowboxPhysics()
    states=[GrowboxState.initial(air_temperature_c=22+i, rh_pct=40+i*7) for i in range(4)]
    forcings=[GrowboxForcing(lamp_dimmer=i/4, exhaust_fan=.5, circulation_fan=.9,
                            cooler=.1*i, co2_doser=.4) for i in range(4)]
    _check_oracle([p]*4, states, forcings, dt=4, ticks=15,
                  dtype=torch.float32, absolute=.004, relative=1e-4)


def test_fail_closed_missing_device_and_invalid_inputs():
    p=GrowboxPhysics(); s=GrowboxState.initial(); f=GrowboxForcing()
    with pytest.raises(ValueError,match='equal-length'):
        simulate_batch([p],[s,s],[f],dt_s=10,ticks=1)
    with pytest.raises(ValueError,match='ticks'):
        simulate_batch([p],[s],[f],dt_s=10,ticks=0)
    with pytest.raises(ValueError,match='float64'):
        simulate_batch([p],[s],[f],dt_s=10,ticks=1,dtype=torch.float16)
    with pytest.raises(RuntimeError,match='unavailable'):
        simulate_batch([p],[s],[f],dt_s=10,ticks=1,device='cuda:999')
    with pytest.raises(ValueError,match='actuator fraction'):
        simulate_batch([p],[s],[GrowboxForcing(lamp_dimmer=1.5)],dt_s=10,ticks=1)
