"""Algorithm-only tests with a deliberately small fake; real-physics tests are separate."""
from dataclasses import dataclass, replace
from types import SimpleNamespace
from unittest.mock import patch
import unittest
from tools.ml.mpc import Command, DeviceState, MPCConfig, Targets, optimize

@dataclass(frozen=True)
class FakeForcing:
    heater: float = 0.
    cooler: float = 0.
    humidifier: float = 0.
    dehumidifier: float = 0.
    exhaust_fan: float = 0.
    circulation_fan: float = 0.
    co2_doser: float = 0.
    lamp_dimmer: float = 0.

@dataclass(frozen=True)
class FakeState:
    air_temperature_c: float = 20.
    co2_ppm: float = 450.
    rh_pct: float = 50.
    cumulative_dli_mol_m2: float = 0.

class FakeModel:
    def __init__(self, state, applied):
        self.state = state
        self.applied = applied
    def step(self, forcing, dt):
        a = self.applied
        self.state = replace(self.state,
            air_temperature_c=self.state.air_temperature_c + dt / 60 * (a.heater * 2 - a.cooler * 2),
            co2_ppm=self.state.co2_ppm + dt / 60 * (a.co2_doser * 180 - a.exhaust_fan * 80),
            rh_pct=self.state.rh_pct + dt / 60 * (a.humidifier * 5 - a.dehumidifier * 5),
            cumulative_dli_mol_m2=self.state.cumulative_dli_mol_m2 + a.lamp_dimmer * dt * 400e-6)
        return self.state
    def observe(self, forcing):
        return SimpleNamespace(rh_pct=self.state.rh_pct, air_vpd_kpa=1,
                               ppfd_umol_m2_s=self.applied.lamp_dimmer * 400)

class MPCMockTest(unittest.TestCase):
    def setUp(self):
        self.physics = SimpleNamespace(equipment=SimpleNamespace(heater_power_w=180.,
             cooler_capacity_w=200., exhaust_max_m3_h=90.),
             lighting=SimpleNamespace(rated_power_w=120., photon_efficacy_umol_j=2.3,
                 canopy_capture_fraction=.65), geometry=SimpleNamespace(canopy_area_m2=.7))
        self.forcing = FakeForcing()
        self.config = MPCConfig(step_s=60, horizon_steps=3, beam_width=2,
                                max_evaluations=60, time_limit_s=2., seed=123)
    def fake_measure(self, physics, state, forcing, command):
        model = FakeModel(state, command)
        return model, model.observe(forcing)
    def test_deterministic_control_and_projections(self):
        targets = Targets(temperature_c=24, rh_pct=60, co2_ppm=800,
                          ppfd_umol_m2_s=250)
        with patch('tools.ml.mpc.optimizer._measure', side_effect=self.fake_measure):
            a = optimize(self.physics,FakeState(),self.forcing,targets,DeviceState(),cfg=self.config)
            b = optimize(self.physics,FakeState(),self.forcing,targets,DeviceState(),cfg=self.config)
        self.assertEqual(a.status, 'PASS', a.reason)
        self.assertEqual(a.requested, b.requested)
        self.assertEqual(a.predictions, b.predictions)
        self.assertGreater(a.evaluations, 10)
        self.assertLessEqual(a.evaluations, self.config.max_evaluations)
        self.assertTrue(a.predictions)
    def test_timeout_has_safe_fallback(self):
        with patch('tools.ml.mpc.optimizer._measure', side_effect=self.fake_measure), \
             patch('tools.ml.mpc.optimizer.time.monotonic', side_effect=[0,1,2,3,4]):
            a = optimize(self.physics,FakeState(),self.forcing,
                 Targets(temperature_c=24,rh_pct=60),DeviceState(),cfg=self.config)
        self.assertEqual(a.status,'BLOCKED')
        self.assertEqual(a.requested.heater,0)

if __name__ == '__main__': unittest.main()
