"""Independent directional characterization on the actual uncalibrated R4 plant."""
import math
import unittest
from dataclasses import replace

from tools.ml.physics.growbox_dynamics import (
    GrowboxForcing, GrowboxPhysics, GrowboxState, GrowboxThermodynamics,
)


class EquipmentResponseTest(unittest.TestCase):
    def setUp(self):
        self.physics = GrowboxPhysics()
        self.base = GrowboxState.initial(volume_m3=self.physics.geometry.air_volume_m3,
                                         air_temperature_c=24, rh_pct=60, co2_ppm=1200)
        self.forcing = GrowboxForcing(outside_temperature_c=20,
                                      outside_relative_humidity_pct=50,
                                      outside_co2_ppm=420)

    def run_one(self, **command):
        f = replace(self.forcing, **command)
        model = GrowboxThermodynamics(self.physics, self.base)
        model.step(f, 60)
        self.assertLess(abs(model.water_balance_residual_kg()), 1e-8)
        self.assertTrue(all(math.isfinite(getattr(model.state, key)) for key in
                            ('air_temperature_c', 'co2_ppm', 'air_vapor_kg')))
        return model, model.observe(f)

    def test_heater_and_cooler_have_opposing_thermal_effect(self):
        base, _ = self.run_one()
        hot, _ = self.run_one(heater=1)
        cold, _ = self.run_one(cooler=1)
        self.assertGreater(hot.state.air_temperature_c, base.state.air_temperature_c)
        self.assertLess(cold.state.air_temperature_c, base.state.air_temperature_c)

    def test_humidifier_and_dehumidifier_have_opposing_vapor_effect(self):
        base, _ = self.run_one()
        wet, _ = self.run_one(humidifier=1)
        dry, _ = self.run_one(dehumidifier=1)
        self.assertGreater(wet.state.air_vapor_kg, base.state.air_vapor_kg)
        self.assertLess(dry.state.air_vapor_kg, base.state.air_vapor_kg)
        self.assertGreater(dry.state.dehumidifier_condensate_kg, 0)

    def test_co2_dosing_and_exhaust_have_opposing_co2_effect(self):
        base, _ = self.run_one()
        dosed, _ = self.run_one(co2_doser=1)
        purged, _ = self.run_one(exhaust_fan=1)
        self.assertGreater(dosed.state.co2_ppm, base.state.co2_ppm)
        self.assertLess(purged.state.co2_ppm, base.state.co2_ppm)

    def test_lighting_dli_and_internal_circulation_are_distinct(self):
        base, b = self.run_one()
        lit, l = self.run_one(lamp_dimmer=1)
        mixed, m = self.run_one(circulation_fan=1)
        self.assertGreater(l.ppfd_umol_m2_s, b.ppfd_umol_m2_s)
        self.assertGreater(lit.state.cumulative_dli_mol_m2, base.state.cumulative_dli_mol_m2)
        self.assertEqual(m.exhaust_m3_h, b.exhaust_m3_h)
        self.assertGreater(m.canopy_air_speed_m_s, b.canopy_air_speed_m_s)

    def test_long_run_stays_finite_and_conserves_water(self):
        model = GrowboxThermodynamics(self.physics, self.base)
        forcing = replace(self.forcing, heater=.1, lamp_dimmer=.2,
                          exhaust_fan=.25, circulation_fan=.3)
        for _ in range(24):  # 2 h at five-minute cadence, independent of MPC
            model.step(forcing, 300)
        self.assertLess(abs(model.water_balance_residual_kg()), 1e-7)
        self.assertGreaterEqual(model.observe(forcing).rh_pct, 0)
        self.assertLessEqual(model.observe(forcing).rh_pct, 100.01)


if __name__ == '__main__':
    unittest.main()
