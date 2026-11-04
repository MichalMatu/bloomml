"""Scenario inventory is deterministic, diverse and explicitly offline."""
import unittest
from tools.ml.mpc.scenario_matrix import SCENARIOS, Scenario


class ScenarioCatalogTest(unittest.TestCase):
    def test_unique_fault_and_weather_scenarios(self):
        self.assertEqual(len({s.name for s in SCENARIOS}), len(SCENARIOS))
        self.assertGreaterEqual(len(SCENARIOS), 10)
        self.assertTrue(any(s.heater_failure for s in SCENARIOS))
        self.assertTrue(any(s.stale_sensor for s in SCENARIOS))
        self.assertTrue(any(s.low_capacity for s in SCENARIOS))
        self.assertTrue(any(s.first_out_t != s.second_out_t for s in SCENARIOS))
        self.assertTrue(any(s.first_out_rh != s.second_out_rh for s in SCENARIOS))
        self.assertTrue(any(s.first_out_co2 != s.second_out_co2 for s in SCENARIOS))
        self.assertTrue(any(s.first_setpoint_t != s.second_setpoint_t for s in SCENARIOS))

    def test_forcing_is_inside_reference_model_domain(self):
        for s in SCENARIOS:
            self.assertTrue(all(-40<=n<=80 for n in (s.first_out_t,s.second_out_t)))
            self.assertTrue(all(0<=n<=100 for n in (s.first_out_rh,s.second_out_rh)))
            self.assertTrue(all(10<=n<=40 for n in (s.first_setpoint_t,s.second_setpoint_t)))
            self.assertTrue(all(300<=n<=1800 for n in (s.target_co2,)))


if __name__=='__main__':unittest.main()
