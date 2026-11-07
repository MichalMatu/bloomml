from dataclasses import replace

import pytest

from tools.ml.physics.growbox_dynamics import (
    GrowboxPhysics, GrowboxGeometry, GrowboxEquipment, GrowboxLighting,
    GrowboxBiology, GrowboxForcing, GrowboxState, GrowboxThermodynamics,
)
from tools.ml.physics.growbox_psychrometrics import (
    dew_point_c, saturation_pressure_pa, vapor_density_kg_m3, vapor_pressure_pa,
)


def plant(*, state=None, physics=None):
    p = physics or GrowboxPhysics()
    s = state or GrowboxState.initial(volume_m3=p.geometry.air_volume_m3)
    return GrowboxThermodynamics(p, s)


def test_psychrometric_reference_and_dewpoint():
    pressure = saturation_pressure_pa(25)
    assert 3150 < pressure < 3200
    assert dew_point_c(0) is None
    assert dew_point_c(pressure * 0.6) == pytest.approx(16.7, abs=0.3)
    assert vapor_density_kg_m3(25, 60) == pytest.approx(0.0138, abs=.0005)


def test_lamp_power_is_partitioned_without_double_counting():
    params = GrowboxPhysics(
        geometry=GrowboxGeometry(wall_air_conductance_w_k=0, wall_outside_conductance_w_k=0,
                                leaf_air_conductance_w_k=0, substrate_air_conductance_w_k=0),
        equipment=GrowboxEquipment(infiltration_ach=0, heater_power_w=0,
                                   fan_electrical_heat_w=0),
        biology=GrowboxBiology(maximum_transpiration_g_h=0,
                               maximum_co2_assimilation_umol_m2_s=0,
                               respiration_umol_m2_s=0),
    )
    sim = plant(physics=params)
    f = GrowboxForcing(outside_temperature_c=24, outside_relative_humidity_pct=60,
                       lamp_dimmer=1, canopy_measured_ppfd_umol_m2_s=600)
    sim.step(f, 10)
    assert sim.state.air_temperature_c > 24
    assert sim.state.wall_temperature_c > 24
    assert sim.state.leaf_temperature_c > 24
    assert sim.observe(f).ppfd_umol_m2_s == 600
    assert abs(sim.water_balance_residual_kg()) < 1e-10


def test_warm_wet_air_and_cold_wall_make_condensate():
    s = GrowboxState.initial(air_temperature_c=25, rh_pct=85, wall_temperature_c=10)
    sim = plant(state=s)
    forcing = GrowboxForcing(outside_temperature_c=25, outside_relative_humidity_pct=85)
    assert sim.observe(forcing).wall_condensation_risk
    sim.step(forcing, 120)
    assert sim.state.wall_condensate_kg > 0
    assert abs(sim.water_balance_residual_kg()) < 1e-8


def test_mass_conservation_many_fluxes_with_cooling_and_transpiration():
    sim = plant()
    forcing = GrowboxForcing(outside_temperature_c=18, outside_relative_humidity_pct=80,
                           lamp_dimmer=.8, exhaust_fan=.4, humidifier=.2, cooler=.4,
                           dehumidifier=.15, co2_doser=.3, irrigation_kg_s=1e-5)
    for _ in range(120):
        sim.step(forcing, 10)
    assert abs(sim.water_balance_residual_kg()) < 1e-8
    assert sim.state.elapsed_s == pytest.approx(1200)
    assert 0 <= sim.observe(forcing).rh_pct <= 100.01


def test_air_exchange_at_90_m3_h_in_08_m3_is_fast():
    p = GrowboxPhysics(geometry=GrowboxGeometry(wall_air_conductance_w_k=0,
        wall_outside_conductance_w_k=0, leaf_air_conductance_w_k=0,
        substrate_air_conductance_w_k=0),
        equipment=GrowboxEquipment(infiltration_ach=0, fan_electrical_heat_w=0),
        biology=GrowboxBiology(maximum_transpiration_g_h=0,
            maximum_co2_assimilation_umol_m2_s=0, respiration_umol_m2_s=0))
    sim = plant(physics=p, state=GrowboxState.initial(co2_ppm=1200))
    f = GrowboxForcing(outside_co2_ppm=420, exhaust_fan=1)
    sim.step(f, 32)
    assert 660 < sim.state.co2_ppm < 750  # 1 nominal air exchange tau


def test_transpiration_depends_on_light_water_and_leaf_vpd():
    wet = GrowboxState.initial(air_temperature_c=25, rh_pct=45, substrate_water_kg=2)
    dry = replace(wet, substrate_water_kg=.02)
    w = plant(state=wet)
    d = plant(state=dry)
    f = GrowboxForcing(lamp_dimmer=1, outside_temperature_c=25, outside_relative_humidity_pct=45)
    w.step(f, 200)
    d.step(f, 200)
    assert wet.substrate_water_kg - w.state.substrate_water_kg > dry.substrate_water_kg - d.state.substrate_water_kg
    assert abs(w.water_balance_residual_kg()) < 1e-8


def test_pressure_changes_air_density_without_making_fake_measurement():
    sim = plant()
    sea = sim.observe(GrowboxForcing(atmospheric_pressure_pa=101325))
    high_alt = sim.observe(GrowboxForcing(atmospheric_pressure_pa=82000))
    assert high_alt.air_density_kg_m3 < sea.air_density_kg_m3
    assert high_alt.dew_point_c == pytest.approx(sea.dew_point_c)


def test_unphysical_parameters_fail_fast():
    with pytest.raises(ValueError):
        GrowboxLighting(air_fraction=.9, leaf_fraction=.3, wall_fraction=.3)
    with pytest.raises(ValueError):
        GrowboxForcing(heater=1.2).validate()
    with pytest.raises(ValueError):
        plant().step(GrowboxForcing(), -1)


def test_measured_lamp_ppfd_and_energy_remain_separate():
    """PPFD is a light meter reading, not an extra unaccounted heat source."""
    a = plant()
    b = plant()
    fa = GrowboxForcing(lamp_measured_electrical_w=120,
                        canopy_measured_ppfd_umol_m2_s=300)
    fb = replace(fa, canopy_measured_ppfd_umol_m2_s=900)
    a.step(fa, 60)
    b.step(fb, 60)
    assert b.state.cumulative_dli_mol_m2 == pytest.approx(0.054)
    assert a.state.cumulative_dli_mol_m2 == pytest.approx(0.018)
    assert a.state.air_temperature_c != pytest.approx(b.state.air_temperature_c)  # biological feedback
    assert abs(a.water_balance_residual_kg()) < 1e-8
    assert abs(b.water_balance_residual_kg()) < 1e-8


def test_leaf_surface_below_dewpoint_collects_condensate():
    state = GrowboxState.initial(air_temperature_c=25, rh_pct=90, leaf_temperature_c=10)
    model = plant(state=state)
    forcing = GrowboxForcing(outside_temperature_c=25, outside_relative_humidity_pct=90)
    assert model.observe(forcing).leaf_condensation_risk
    model.step(forcing, 30)
    assert model.state.leaf_condensate_kg > 0
    assert abs(model.water_balance_residual_kg()) < 1e-8


def test_finite_water_drain_is_accounted_for():
    p = GrowboxPhysics(biology=GrowboxBiology(substrate_water_capacity_kg=1))
    initial = GrowboxState.initial(substrate_water_kg=.95)
    sim = plant(physics=p, state=initial)
    sim.step(GrowboxForcing(irrigation_kg_s=.01), 10)
    assert sim.state.substrate_water_kg == pytest.approx(1.0)
    assert sim.state.drainage_kg > 0
    assert abs(sim.water_balance_residual_kg()) < 1e-9


def test_light_solar_absorbed_by_wall_even_opaque_growbox():
    p = GrowboxPhysics(lighting=GrowboxLighting(solar_window_area_m2=0))
    dark, lit = plant(physics=p), plant(physics=p)
    f = GrowboxForcing(solar_irradiance_w_m2=500)
    dark.step(GrowboxForcing(), 60)
    lit.step(f, 60)
    assert lit.state.wall_temperature_c > dark.state.wall_temperature_c
    assert lit.observe(f).ppfd_umol_m2_s == 0
    assert abs(lit.water_balance_residual_kg()) < 1e-8


def test_physics_timestep_convergence():
    f = GrowboxForcing(lamp_dimmer=.7, heater=.4, exhaust_fan=.35,
                       outside_temperature_c=15, outside_relative_humidity_pct=70)
    fine, coarse = plant(), plant()
    for _ in range(200):
        fine.step(f, 1)
    for _ in range(20):
        coarse.step(f, 10)
    assert abs(fine.state.air_temperature_c - coarse.state.air_temperature_c) < .2
    assert abs(fine.observe(f).rh_pct - coarse.observe(f).rh_pct) < .5
    assert abs(fine.state.co2_ppm - coarse.state.co2_ppm) < 3


def test_closed_lamp_energy_addition_is_conserved_at_short_dt():
    cfg = GrowboxPhysics(
        geometry=GrowboxGeometry(wall_air_conductance_w_k=0, wall_outside_conductance_w_k=0,
                                leaf_air_conductance_w_k=0, substrate_air_conductance_w_k=0,
                                wall_vapor_transfer_m_s=0, leaf_vapor_transfer_m_s=0,
                                substrate_evaporation_kg_m2_s=0),
        biology=GrowboxBiology(maximum_transpiration_g_h=0,
                               maximum_co2_assimilation_umol_m2_s=0,
                               respiration_umol_m2_s=0),
        equipment=GrowboxEquipment(infiltration_ach=0, fan_electrical_heat_w=0),
    )
    sim = plant(physics=cfg)
    old = sim.state
    forcing = GrowboxForcing(lamp_measured_electrical_w=120)
    sim.step(forcing, 1)
    new = sim.state
    density = sim.observe(forcing).air_density_kg_m3
    dE = (density * cfg.geometry.air_volume_m3 * 1005 * (new.air_temperature_c - old.air_temperature_c)
          + cfg.geometry.wall_heat_capacity_j_k * (new.wall_temperature_c - old.wall_temperature_c)
          + cfg.geometry.leaf_heat_capacity_j_k * (new.leaf_temperature_c - old.leaf_temperature_c)
          + cfg.geometry.substrate_heat_capacity_j_k * (new.substrate_temperature_c - old.substrate_temperature_c))
    assert dE == pytest.approx(120.0, abs=0.5)
