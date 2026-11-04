"""Real R4 simulation evidence structure, fault truth and UNKNOWN gate."""
import unittest
from tools.ml.physics.growbox_dynamics import GrowboxPhysics, GrowboxState, GrowboxForcing
from tools.ml.mpc import MPCConfig, Targets, run_episode

SHA='1feaa6af3d2657faa3a3cbb54ee7cfcc24cc6d2a'

class ObservabilityTest(unittest.TestCase):
    def setUp(self):
        self.physics=GrowboxPhysics()
        self.state=GrowboxState.initial(volume_m3=self.physics.geometry.air_volume_m3,
                                         air_temperature_c=22,rh_pct=55,co2_ppm=450)
        self.forcing=lambda t: GrowboxForcing()
        self.targets=lambda t: Targets(temperature_c=24,rh_pct=60,co2_ppm=700,
                                         ppfd_umol_m2_s=100)
        self.config=MPCConfig(step_s=15,horizon_steps=2,max_evaluations=18,time_limit_s=30)

    def test_predictions_applied_truth_and_water_ledger(self):
        m,rows=run_episode(self.physics,self.state,self.forcing,self.targets,steps=2,
                            cfg=self.config,source_commit=SHA)
        self.assertEqual(len(rows),2)
        self.assertTrue(all(r['source_commit']==SHA for r in rows))
        self.assertTrue(all(r['predictions'] for r in rows))
        self.assertTrue(all(r['one_step_prediction_error'] is not None for r in rows))
        self.assertTrue(all(r['prediction_uncertainty']=='synthetic_uncalibrated' for r in rows))
        self.assertTrue(all(r['optimizer_evaluations']>0 for r in rows))
        self.assertTrue(all(r['virtual_execution_status']=='simulated_applied' for r in rows))
        self.assertTrue(all(abs(r['water_residual_kg'])<1e-8 for r in rows))

    def test_stale_reading_and_fault_are_not_hidden(self):
        m,rows=run_episode(self.physics,self.state,self.forcing,self.targets,steps=2,
                         cfg=self.config,missing_at=lambda _:500,
                         faulted_at=lambda _:frozenset({'co2_doser'}),source_commit=SHA)
        self.assertEqual(m.status,'FAIL')
        self.assertEqual(m.fallback_steps,2)
        self.assertEqual(rows[0]['status'],'BLOCKED')
        self.assertEqual(rows[0]['applied']['co2_doser'],0)
        self.assertEqual(rows[0]['virtual_device_faults'],['co2_doser'])
        self.assertEqual(rows[0]['virtual_execution_status'], 'faulted_simulation')

    def test_without_source_sha_gate_is_unknown_not_pass(self):
        m,_=run_episode(self.physics,self.state,self.forcing,self.targets,
                         steps=1,cfg=self.config,policy='threshold')
        self.assertEqual(m.status,'UNKNOWN')

    def test_missing_sha_with_actual_failure_is_fail_not_unknown(self):
        m,_=run_episode(self.physics,self.state,self.forcing,self.targets,steps=1,
                         cfg=self.config,missing_at=lambda _:500)
        self.assertEqual(m.status,'FAIL')

if __name__=='__main__':unittest.main()
