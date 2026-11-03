"""Checkpoint: physical conservation, mixed convection, sensor edge cases.

No external hardware, firmware, or training authority is touched.
"""
from __future__ import annotations

from dataclasses import replace
import math

import pytest

from tools.ml.physics.growbox_convection import (
    effective_canopy_air_speed_m_s,
    mixed_convection_factor,
)
from tools.ml.physics.growbox_dynamics import (
    GrowboxBiology, GrowboxEquipment, GrowboxForcing, GrowboxGeometry,
    GrowboxPhysics, GrowboxState, GrowboxThermodynamics,
)
from tools.ml.physics.growbox_psychrometrics import (
    LATENT_HEAT_VAPORIZATION, moist_air_density_kg_m3,
)


def _isolated(*, air=22.0, wall=22.0, leaf=32.0, soil=22.0):
    geometry = GrowboxGeometry(
        wall_air_conductance_w_k=15,
        wall_outside_conductance_w_k=0,
        leaf_air_conductance_w_k=12,
        substrate_air_conductance_w_k=8,
        wall_vapor_transfer_m_s=0,
        leaf_vapor_transfer_m_s=0,
        substrate_evaporation_kg_m2_s=0,
        external_solar_area_m2=0,
    )
    biology = GrowboxBiology(
        maximum_transpiration_g_h=0,
        maximum_co2_assimilation_umol_m2_s=0,
        respiration_umol_m2_s=0,
    )
    equipment = GrowboxEquipment(
        infiltration_ach=0, heater_power_w=0, cooler_capacity_w=0,
        circulation_fan_heat_w=0, fan_electrical_heat_w=0,
    )
    params = GrowboxPhysics(geometry=geometry, biology=biology, equipment=equipment)
    state = GrowboxState.initial(
        air_temperature_c=air, wall_temperature_c=wall,
        leaf_temperature_c=leaf, substrate_temperature_c=soil,
        volume_m3=geometry.air_volume_m3,
    )
    return GrowboxThermodynamics(params, state)


def test_internal_circulation_accelerates_leaf_to_air_heat_without_outdoor_exchange():
    idle = _isolated()
    mixed = _isolated()
    f0 = GrowboxForcing(outside_temperature_c=22, outside_relative_humidity_pct=60)
    f1 = replace(f0, circulation_fan=1)
    idle.step(f0, 20)
    mixed.step(f1, 20)
    assert mixed.state.leaf_temperature_c < idle.state.leaf_temperature_c
    assert mixed.state.air_temperature_c > idle.state.air_temperature_c
    assert idle.observe(f0).exhaust_m3_h == mixed.observe(f1).exhaust_m3_h == 0
    assert mixed.observe(f1).canopy_air_speed_m_s > idle.observe(f0).canopy_air_speed_m_s
    assert abs(mixed.water_balance_residual_kg()) < 1e-9


def test_internal_circulation_changes_wall_and_substrate_heat_transfer():
    idle = _isolated(air=20, wall=35, leaf=20, soil=30)
    mixed = _isolated(air=20, wall=35, leaf=20, soil=30)
    idle.step(GrowboxForcing(outside_temperature_c=20), 120)
    mixed.step(GrowboxForcing(outside_temperature_c=20, circulation_fan=1), 120)
    assert mixed.state.wall_temperature_c < idle.state.wall_temperature_c
    assert mixed.state.substrate_temperature_c < idle.state.substrate_temperature_c


def test_natural_convection_needs_no_fan():
    f = mixed_convection_factor(0, 32, 20, characteristic_length_m=0.1)
    assert math.isfinite(f) and f > 0
    assert mixed_convection_factor(0, 20, 20, characteristic_length_m=0.1) == 0
    assert mixed_convection_factor(1.0, 32, 20, characteristic_length_m=0.1) > f


def test_local_speed_measurement_overrides_empirical_estimation():
    f = GrowboxForcing(measured_air_speed_m_s=0, circulation_fan=1)
    observed = _isolated().observe(f)
    assert observed.canopy_air_speed_m_s == 0
    assert observed.canopy_air_speed_source == "measured"
    auto = _isolated().observe(GrowboxForcing(circulation_fan=1))
    assert auto.canopy_air_speed_source == "estimated"
    assert auto.canopy_air_speed_m_s > 0


def test_circulation_fan_electrical_heat_is_counted_once():
    cfg = GrowboxPhysics(
        geometry=GrowboxGeometry(
            wall_air_conductance_w_k=0, wall_outside_conductance_w_k=0,
            leaf_air_conductance_w_k=0, substrate_air_conductance_w_k=0,
            wall_vapor_transfer_m_s=0, leaf_vapor_transfer_m_s=0,
            substrate_evaporation_kg_m2_s=0, external_solar_area_m2=0),
        biology=GrowboxBiology(maximum_transpiration_g_h=0,
             maximum_co2_assimilation_umol_m2_s=0, respiration_umol_m2_s=0),
        equipment=GrowboxEquipment(infiltration_ach=0, fan_electrical_heat_w=0,
             circulation_fan_heat_w=5),
    )
    sim = GrowboxThermodynamics(cfg, GrowboxState.initial())
    f = GrowboxForcing(circulation_fan=1)
    old = sim.state
    sim.step(f, 1)
    cp_air = moist_air_density_kg_m3(24, 101325,
        sim.observe(f).partial_vapor_pressure_pa)*cfg.geometry.air_volume_m3*1005
    assert cp_air * (sim.state.air_temperature_c-old.air_temperature_c) == pytest.approx(5, abs=0.01)


def test_energy_exchange_conserves_sensible_heat_without_external_or_latent_sources():
    sim = _isolated(air=20, wall=33, leaf=35, soil=27)
    f = GrowboxForcing(outside_temperature_c=20, outside_relative_humidity_pct=60)
    s0 = sim.state
    density = moist_air_density_kg_m3(s0.air_temperature_c, 101325,
        sim.observe(f).partial_vapor_pressure_pa)
    g = sim.parameters.geometry
    def sensible_energy(state):
        return (density*g.air_volume_m3*1005*state.air_temperature_c
                +g.wall_heat_capacity_j_k*state.wall_temperature_c
                +g.leaf_heat_capacity_j_k*state.leaf_temperature_c
                +g.substrate_heat_capacity_j_k*state.substrate_temperature_c)
    e0 = sensible_energy(s0)
    sim.step(f, 30)
    assert abs(sensible_energy(sim.state)-e0) < 1e-5


def test_out_of_range_late_substep_keeps_state_atomic():
    cfg = GrowboxPhysics(equipment=GrowboxEquipment(heater_power_w=6000, infiltration_ach=0))
    initial = GrowboxState.initial(air_temperature_c=62)
    sim = GrowboxThermodynamics(cfg, initial)
    with pytest.raises(ValueError):
        sim.step(GrowboxForcing(heater=1), 20)
    assert sim.state == initial


def test_invalid_circulation_and_measured_speed_rejected():
    sim = _isolated()
    with pytest.raises(ValueError):
        sim.step(GrowboxForcing(circulation_fan=-0.01), 1)
    with pytest.raises(ValueError):
        sim.observe(GrowboxForcing(measured_air_speed_m_s=float("nan")))
    with pytest.raises(ValueError):
        effective_canopy_air_speed_m_s(measured_air_speed_m_s=-1,
            exhaust_command=0, circulation_command=0,
            background_air_speed_m_s=0.1,
            exhaust_induced_air_speed_m_s=0.2,
            circulation_air_speed_m_s=0.6)


def test_flux_conserves_mass_under_high_circulation_and_condensation():
    state = GrowboxState.initial(air_temperature_c=27, rh_pct=92,
                                 wall_temperature_c=14, leaf_temperature_c=18)
    sim = GrowboxThermodynamics(GrowboxPhysics(), state)
    f = GrowboxForcing(circulation_fan=1, exhaust_fan=0.7,
        outside_temperature_c=8, outside_relative_humidity_pct=90,
        lamp_dimmer=0.5, dehumidifier=0.3)
    for _ in range(100):
        sim.step(f, 10)
    assert abs(sim.water_balance_residual_kg()) < 1e-8
    assert math.isfinite(sim.state.air_temperature_c)
    assert sim.state.air_vapor_kg >= 0


def test_air_exchange_never_confuses_internal_circulation_with_exhaust():
    f0 = GrowboxForcing(outside_co2_ppm=400)
    f1 = replace(f0, circulation_fan=1)
    a, b = _isolated(), _isolated()
    a.step(f0, 100)
    b.step(f1, 100)
    assert a.state.co2_ppm == pytest.approx(b.state.co2_ppm)


def test_latent_plus_sensible_energy_is_closed_in_isolated_wet_chamber():
    g = GrowboxGeometry(wall_outside_conductance_w_k=0, external_solar_area_m2=0)
    eq = GrowboxEquipment(infiltration_ach=0, fan_electrical_heat_w=0,
                          circulation_fan_heat_w=0)
    params = GrowboxPhysics(geometry=g, equipment=eq)
    initial = GrowboxState.initial(
        air_temperature_c=25, rh_pct=85,
        wall_temperature_c=15, leaf_temperature_c=27)
    sim = GrowboxThermodynamics(params, initial)
    def energy(state):
        return (sim._reference_air_density_kg_m3*g.air_volume_m3*1005*state.air_temperature_c
                +g.wall_heat_capacity_j_k*state.wall_temperature_c
                +g.leaf_heat_capacity_j_k*state.leaf_temperature_c
                +g.substrate_heat_capacity_j_k*state.substrate_temperature_c
                +state.air_vapor_kg*LATENT_HEAT_VAPORIZATION)
    initial_energy = energy(initial)
    sim.step(GrowboxForcing(), 100)
    assert abs(energy(sim.state) - initial_energy) < 1e-5
    assert abs(sim.water_balance_residual_kg()) < 1e-9


def test_circulation_timestep_convergence():
    f = GrowboxForcing(circulation_fan=0.8, exhaust_fan=0.2,
                       outside_temperature_c=12, outside_relative_humidity_pct=80,
                       lamp_dimmer=0.5)
    fine = GrowboxThermodynamics(GrowboxPhysics(), GrowboxState.initial())
    coarse = GrowboxThermodynamics(GrowboxPhysics(), GrowboxState.initial())
    for _ in range(120):
        fine.step(f, 1)
    for _ in range(12):
        coarse.step(f, 10)
    assert abs(fine.state.air_temperature_c - coarse.state.air_temperature_c) < 0.05
    assert abs(fine.observe(f).rh_pct - coarse.observe(f).rh_pct) < 0.1
    assert abs(fine.state.co2_ppm - coarse.state.co2_ppm) < 0.5
