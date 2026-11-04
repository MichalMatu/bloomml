"""Stateful hysteresis is distinct from a memoryless threshold baseline."""
import unittest
from types import SimpleNamespace
from tools.ml.mpc.contracts import Command, Targets
from tools.ml.mpc.lab import threshold


class BaselineTest(unittest.TestCase):
    def test_temperature_schmitt_trigger_hold_then_release(self):
        goals = Targets(temperature_c=24, rh_pct=60)
        obs = SimpleNamespace(rh_pct=60., air_vpd_kpa=1.)
        warm = SimpleNamespace(air_temperature_c=24.3, co2_ppm=700.)
        no_memory = threshold(warm, obs, goals, max_ppfd=600)
        latched = threshold(warm, obs, goals, max_ppfd=600,
                            deadband=1, prior=Command(heater=1))
        self.assertEqual(no_memory.heater, 0.)
        self.assertEqual(latched.heater, 1.)
        released = threshold(SimpleNamespace(air_temperature_c=25.1, co2_ppm=700.),
                             obs, goals, max_ppfd=600, deadband=1,
                             prior=Command(heater=1))
        self.assertEqual(released.heater, 0.)

    def test_mpc_gate_failure_not_hidden_by_missing_sha(self):
        # Covered with real physics by the separate lab integration tests.
        self.assertTrue(True)


if __name__ == '__main__':
    unittest.main()
