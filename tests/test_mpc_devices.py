"""Unit tests for MPC contracts and standalone virtual-device interlocks."""
import unittest
from dataclasses import replace
from tools.ml.mpc.contracts import Command, MPCConfig, Targets
from tools.ml.mpc.devices import DeviceState, DeviceLimits, safe_request, transition

class DeviceContractTest(unittest.TestCase):
    def test_invalid_targets_and_actions_fail(self):
        with self.assertRaises(ValueError): Targets(temperature_c=23)
        with self.assertRaises(ValueError): Targets(temperature_c=23, rh_pct=60, vpd_kpa=1.2)
        with self.assertRaises(ValueError): Command(heater=float('nan'))
        with self.assertRaises(ValueError): Command(heater=-.1)
        with self.assertRaises(ValueError): MPCConfig(horizon_steps=0)

    def test_conflict_policy(self):
        request = Command(heater=1, cooler=1, humidifier=1, dehumidifier=1,
                          co2_doser=1, exhaust_fan=1)
        filtered, reasons = safe_request(request, temperature_c=25, rh_pct=60, co2_ppm=800)
        self.assertEqual((filtered.cooler, filtered.dehumidifier, filtered.exhaust_fan), (0,0,0))
        self.assertIn('heat_cool_conflict', reasons)

    def test_hard_stop_overrides_dwell_and_slew(self):
        state = DeviceState()
        device, rejected = transition(state, Command(heater=1), 10,
                                      DeviceLimits(slew_fraction_per_s=.01))
        self.assertAlmostEqual(device.applied.heater, .1)
        device, rejected = transition(device, Command(heater=0), 10,
                                      DeviceLimits(slew_fraction_per_s=.01))
        self.assertAlmostEqual(device.applied.heater, .1)
        self.assertIn('heater:minimum_on', rejected)
        device, _ = transition(device, Command(heater=0), 10,
                               emergency_stop=frozenset({'heater'}))
        self.assertEqual(device.applied.heater, 0)

    def test_faulted_device_does_not_masquerade_as_applied(self):
        device, rejected = transition(DeviceState(), Command(co2_doser=1), 60,
                                      faulted=frozenset({'co2_doser'}))
        self.assertEqual(device.applied.co2_doser, 0)
        self.assertIn('co2_doser:failed', rejected)

    def test_dwell_conflict_cannot_energize_opposites(self):
        state, _ = transition(DeviceState(), Command(heater=1), 20)
        next_state, rejected = transition(state, Command(cooler=1), 20)
        self.assertFalse(next_state.applied.heater > 0 and next_state.applied.cooler > 0)
        self.assertIn('cooler:interlock', rejected)

    def test_envelope_stops_unsafe_command(self):
        req, _ = safe_request(Command(heater=1, lamp_dimmer=1, humidifier=1,
                                      co2_doser=1), temperature_c=37, rh_pct=92,
                              co2_ppm=1900)
        self.assertEqual(req, Command())

if __name__ == '__main__': unittest.main()
