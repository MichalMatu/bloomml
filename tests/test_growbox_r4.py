"""R4 validation: phase equilibrium, fan provenance and CPU/tensor parity."""
from dataclasses import replace
import math

import pytest

torch = pytest.importorskip('torch')

from tools.ml.physics.growbox_dynamics import (
    GrowboxEquipment, GrowboxForcing, GrowboxPhysics, GrowboxState,
    GrowboxThermodynamics,
)
from tools.ml.physics.growbox_phase import phase_equilibrium
from tools.ml.physics.growbox_psychrometrics import (
    LATENT_HEAT_VAPORIZATION, saturation_vapor_density_kg_m3,
)
from tools.ml.physics.growbox_tensor import simulate_batch


def test_hot_supersaturation_equilibrates_with_no_phantom_energy():
    volume, heat_capacity, initial_temp = .8, 1000., 20.
    vapor = saturation_vapor_density_kg_m3(initial_temp) * volume * 3.
    after_vapor, after_fog, after_temp = phase_equilibrium(
        vapor_kg=vapor, fog_kg=0., air_temperature_c=initial_temp,
        volume_m3=volume, air_capacity_j_k=heat_capacity, dt_s=2.)
    assert after_temp > initial_temp
    assert after_fog > 0
    assert after_vapor + after_fog == pytest.approx(vapor, abs=1e-14)
    assert after_vapor / (saturation_vapor_density_kg_m3(after_temp) * volume) == pytest.approx(1, abs=1e-12)
    assert heat_capacity * (after_temp - initial_temp) + LATENT_HEAT_VAPORIZATION * (after_vapor-vapor) == pytest.approx(0, abs=1e-8)


def test_fog_re_evaporates_by_elapsed_time_not_number_of_substeps():
    volume, heat_capacity, initial_temp = .8, 1000., 23.
    vapor = saturation_vapor_density_kg_m3(initial_temp) * volume * .3
    params = dict(air_temperature_c=initial_temp, volume_m3=volume, air_capacity_j_k=heat_capacity)
    once = phase_equilibrium(vapor_kg=vapor, fog_kg=.003, dt_s=2., **params)
    m, fog, temp = vapor, .003, initial_temp
    for _ in range(10):
        m, fog, temp = phase_equilibrium(vapor_kg=m, fog_kg=fog, air_temperature_c=temp,
                volume_m3=volume, air_capacity_j_k=heat_capacity, dt_s=.2)
    assert once[0] == pytest.approx(m, rel=1e-9, abs=1e-12)
    assert once[2] == pytest.approx(temp, rel=1e-9, abs=1e-10)


def test_curve_only_if_characterized_and_measured_flow_takes_priority():
    p = GrowboxPhysics(equipment=replace(GrowboxEquipment(), exhaust_shutoff_pressure_pa=70.))
    initial = GrowboxState.initial()
    sim = GrowboxThermodynamics(p, initial)
    forcing = GrowboxForcing(exhaust_fan=1, measured_pressure_difference_pa=-35)
    observation = sim.observe(forcing)
    assert observation.exhaust_m3_h == pytest.approx(90/math.sqrt(2))
    assert observation.exhaust_source == 'characterized_curve_estimate'
    assert sim.observe(replace(forcing, measured_pressure_difference_pa=-70)).exhaust_m3_h == 0
    assert sim.observe(replace(forcing, measured_exhaust_m3_h=33)).exhaust_m3_h == 33
    assert sim.observe(replace(forcing, measured_exhaust_m3_h=33)).exhaust_source == 'measured'
    uncharacterized = GrowboxThermodynamics(GrowboxPhysics(),initial)
    assert uncharacterized.observe(forcing).exhaust_m3_h == 90
    assert uncharacterized.observe(forcing).exhaust_source == 'nameplate_estimate'


def test_extreme_wet_surface_cpu_tensor_longer_parity():
    p = GrowboxPhysics()
    state = GrowboxState.initial(air_temperature_c=28, rh_pct=99.9,
        wall_temperature_c=10, leaf_temperature_c=14)
    forcing = GrowboxForcing(circulation_fan=.8, humidifier=.8, cooler=.6,
        outside_temperature_c=8, outside_relative_humidity_pct=95,
        exhaust_fan=.4)
    reference = GrowboxThermodynamics(p,state)
    for _ in range(20):
        reference.step(forcing,2)
    actual = simulate_batch([p],[state],[forcing],device='cpu',dtype=torch.float64,dt_s=2,ticks=20).states[0]
    assert actual.air_temperature_c == pytest.approx(reference.state.air_temperature_c,abs=2e-7)
    assert actual.air_vapor_kg == pytest.approx(reference.state.air_vapor_kg,abs=2e-9)
    assert actual.fog_condensate_kg == pytest.approx(reference.state.fog_condensate_kg,abs=2e-9)
    assert abs(reference.water_balance_residual_kg()) < 1e-9


def test_cpu_tensor_parity_with_pressure_curve_and_isothermal_moist_exchange():
    p = GrowboxPhysics(equipment=replace(GrowboxEquipment(),exhaust_shutoff_pressure_pa=120.))
    s = GrowboxState.initial(air_temperature_c=22,rh_pct=80)
    f = GrowboxForcing(outside_temperature_c=22,outside_relative_humidity_pct=20,
        exhaust_fan=.75, measured_pressure_difference_pa=-40.)
    scalar = GrowboxThermodynamics(p,s)
    for _ in range(6): scalar.step(f,5)
    batch = simulate_batch([p],[s],[f],dt_s=5,ticks=6,device='cpu',dtype=torch.float64)
    assert batch.states[0].air_temperature_c == pytest.approx(scalar.state.air_temperature_c,abs=2e-8)
    assert batch.states[0].air_vapor_kg == pytest.approx(scalar.state.air_vapor_kg,abs=2e-10)
