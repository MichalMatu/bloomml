"""Integration on the full R4 physics solver; never uses a substitute plant."""
import unittest
from dataclasses import replace
from tools.ml.physics.growbox_dynamics import GrowboxForcing, GrowboxPhysics, GrowboxState, GrowboxThermodynamics
from tools.ml.mpc import MPCConfig, Targets, DeviceState, optimize, run_episode

class MPCPhysicsIntegrationTest(unittest.TestCase):
    def setUp(self):
        self.physics = GrowboxPhysics()
        self.state = GrowboxState.initial(volume_m3=self.physics.geometry.air_volume_m3,
                                          air_temperature_c=20, rh_pct=55, co2_ppm=450)
        self.forcing = GrowboxForcing(outside_temperature_c=18, outside_relative_humidity_pct=60,
                                      outside_co2_ppm=420)
        self.targets = Targets(temperature_c=24, rh_pct=60, co2_ppm=750, ppfd_umol_m2_s=200)
        self.cfg = MPCConfig(step_s=15, horizon_steps=2, beam_width=2,
                             max_evaluations=18, time_limit_s=30, seed=41)

    def test_mpc_deterministic_and_bounded_with_real_physics(self):
        a = optimize(self.physics,self.state,self.forcing,self.targets, DeviceState(),cfg=self.cfg)
        b = optimize(self.physics,self.state,self.forcing,self.targets, DeviceState(),cfg=self.cfg)
        self.assertEqual(a.status, 'PASS', a.reason)
        self.assertEqual(a.requested, b.requested)
        self.assertEqual(a.predictions, b.predictions)
        self.assertLessEqual(a.evaluations, self.cfg.max_evaluations)
        self.assertTrue(a.predictions)

    def test_sensor_dropout_fails_closed(self):
        result = optimize(self.physics,self.state,self.forcing,self.targets,DeviceState(),
                          cfg=self.cfg, sensor_age_s=500)
        self.assertEqual(result.status,'BLOCKED')
        self.assertEqual(result.requested.heater,0)
        self.assertEqual(result.evaluations,0)

    def test_closed_loop_produces_truthful_metrics_and_water_balance(self):
        params = self.physics
        schedule = lambda _: self.forcing
        goals = lambda _: self.targets
        result, samples = run_episode(params, self.state, schedule, goals, steps=4, cfg=self.cfg)
        self.assertEqual(len(samples),4)
        self.assertLess(result.water_residual_max_kg,1e-7)
        self.assertGreaterEqual(result.energy_proxy_wh,0)
        self.assertTrue(all(r['origin']=='synthetic_uncalibrated' for r in samples))
        self.assertTrue(all(set(r['applied'])==set(r['proposed']) for r in samples))

    def test_longer_thermal_response_without_claiming_calibration(self):
        model = GrowboxThermodynamics(self.physics,self.state)
        cold = self.forcing
        warm = replace(cold,heater=1)
        model.step(warm,30)
        self.assertGreater(model.state.air_temperature_c,self.state.air_temperature_c)
        self.assertLess(abs(model.water_balance_residual_kg()),1e-7)

if __name__ == '__main__': unittest.main()
