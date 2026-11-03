"""Fixed-seed numerical guardrail for plausible extreme conditions."""
from __future__ import annotations

from dataclasses import replace
import math
import random

import pytest

from tools.ml.physics.growbox_dynamics import (
    GrowboxEquipment, GrowboxForcing, GrowboxGeometry, GrowboxLighting,
    GrowboxPhysics, GrowboxState, GrowboxThermodynamics,
)


def test_bounded_extreme_scenarios_mass_and_finiteness():
    rng = random.Random(741335)
    for trial in range(100):
        volume = rng.uniform(0.08, 3.0)
        g = replace(GrowboxGeometry(),
            air_volume_m3=volume,
            wall_air_conductance_w_k=rng.uniform(0, 50),
            leaf_air_conductance_w_k=rng.uniform(0, 30),
            substrate_air_conductance_w_k=rng.uniform(0, 20),
            wall_outside_conductance_w_k=rng.uniform(0, 20),
            wall_heat_capacity_j_k=rng.uniform(5000, 200000),
            leaf_heat_capacity_j_k=rng.uniform(1000, 15000),
            substrate_heat_capacity_j_k=rng.uniform(10000, 200000),
            canopy_area_m2=rng.uniform(0.1, 2.0),
        )
        eq = replace(GrowboxEquipment(),
            exhaust_max_m3_h=rng.uniform(0, 200),
            infiltration_ach=rng.uniform(0, 3.0),
            background_air_speed_m_s=rng.uniform(0, 0.3),
            circulation_reference_air_speed_m_s=rng.uniform(0, 2.0),
        )
        initial_temp = rng.uniform(10, 35)
        initial = GrowboxState.initial(
            volume_m3=volume, air_temperature_c=initial_temp,
            wall_temperature_c=initial_temp + rng.uniform(-8, 8),
            leaf_temperature_c=initial_temp + rng.uniform(-5, 5),
            substrate_temperature_c=initial_temp + rng.uniform(-6, 6),
            rh_pct=rng.uniform(5, 95),
            co2_ppm=rng.uniform(300, 1800),
            substrate_water_kg=rng.uniform(.01, 2.95),
        )
        sim = GrowboxThermodynamics(GrowboxPhysics(
            geometry=g, equipment=eq,
            lighting=replace(GrowboxLighting(), rated_power_w=rng.uniform(0, 220))), initial)
        f = GrowboxForcing(
            outside_temperature_c=rng.uniform(-10, 42),
            outside_relative_humidity_pct=rng.uniform(0, 100),
            outside_co2_ppm=rng.uniform(300, 2000),
            atmospheric_pressure_pa=rng.uniform(80000, 105000),
            lamp_dimmer=rng.random(),
            heater=rng.random(), cooler=rng.random(),
            humidifier=rng.random(), dehumidifier=rng.random(),
            exhaust_fan=rng.random(), circulation_fan=rng.random(),
            co2_doser=rng.random(), irrigation_kg_s=rng.uniform(0, 0.00001),
        )
        for _ in range(12):
            sim.step(f, 10)
        obs = sim.observe(f)
        assert math.isfinite(sim.state.air_temperature_c), trial
        assert math.isfinite(obs.air_vpd_kpa), trial
        assert 0 <= obs.rh_pct <= 100.00001, trial
        assert sim.state.substrate_water_kg >= 0, trial
        assert sim.state.air_vapor_kg >= 0, trial
        assert abs(sim.water_balance_residual_kg()) < 1e-8, trial


def test_pressure_reference_outside_valid_bounds():
    with pytest.raises(ValueError):
        GrowboxGeometry(reference_pressure_pa=35000)
    with pytest.raises(ValueError):
        GrowboxGeometry(reference_pressure_pa=120000)
