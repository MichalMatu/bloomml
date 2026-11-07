"""Research-only Rule/OutputSupervisor witness: no application-side callbacks."""
import unittest
from tools.ml.mpc.contracts import Command
from tools.ml.mpc.shadow_contract import (
    ShadowProposal, RuleVerdict, SupervisorFeedback, evaluate_shadow,
)

SHA = '1feaa6af3d2657faa3a3cbb54ee7cfcc24cc6d2a'


class ShadowContractTest(unittest.TestCase):
    def proposal(self, **overrides):
        fields = dict(proposal_id='p-1',source_commit=SHA,observed_at_s=20.,
                      sensor_valid=True,sensor_age_s=1.,command=Command(heater=.4))
        fields.update(overrides)
        return ShadowProposal(**fields)

    def test_missing_verdict_and_missing_supervisor_evidence_are_blocked(self):
        p=self.proposal()
        self.assertEqual(evaluate_shadow(p,now_s=30).status,'BLOCKED')
        self.assertEqual(evaluate_shadow(p,now_s=30,
            rule_verdict=RuleVerdict('p-1',True,'rule_ok')).reason,'execution_evidence_missing')

    def test_sensor_stale_rejects_before_other_records(self):
        p=self.proposal(sensor_age_s=300)
        r=evaluate_shadow(p,now_s=30,rule_verdict=RuleVerdict('p-1',True,'ok'))
        self.assertEqual(r.status,'BLOCKED')
        self.assertEqual(r.supervisor_status,'NOT_ATTEMPTED')

    def test_rule_rejection_never_execution(self):
        p=self.proposal()
        r=evaluate_shadow(p,now_s=30,rule_verdict=RuleVerdict('p-1',False,'safety'))
        self.assertEqual(r.status,'BLOCKED')
        self.assertFalse(r.rule_accepted)
        self.assertEqual(r.supervisor_status,'NOT_ATTEMPTED')

    def test_transport_sent_is_not_execution_ack(self):
        p=self.proposal()
        r=evaluate_shadow(p,now_s=30,rule_verdict=RuleVerdict('p-1',True,'ok'),
             supervisor_feedback=SupervisorFeedback('p-1','SENT',updated_at_s=25))
        self.assertEqual(r.status,'BLOCKED')
        self.assertFalse(r.execution_acknowledged)

    def test_fake_ack_only_and_no_real_ack(self):
        p=self.proposal()
        r=evaluate_shadow(p,now_s=30,rule_verdict=RuleVerdict('p-1',True,'ok'),
             supervisor_feedback=SupervisorFeedback('p-1','SIMULATED_ACK',updated_at_s=25))
        self.assertEqual(r.status,'PASS')
        self.assertFalse(r.execution_acknowledged)
        self.assertTrue(r.simulation_only)

    def test_timeout_and_mismatched_ids(self):
        p=self.proposal()
        verdict=RuleVerdict('p-1',True,'ok')
        timeout=evaluate_shadow(p,now_s=30,rule_verdict=verdict,
             supervisor_feedback=SupervisorFeedback('p-1','TIMEOUT',updated_at_s=25))
        self.assertEqual(timeout.status,'BLOCKED')
        mismatch=evaluate_shadow(p,now_s=30,rule_verdict=verdict,
             supervisor_feedback=SupervisorFeedback('not-p-1','SENT',updated_at_s=25))
        self.assertEqual(mismatch.status,'FAIL')

    def test_missing_commit_is_unknown_without_other_failure(self):
        p=self.proposal(source_commit='UNKNOWN')
        self.assertEqual(evaluate_shadow(p,now_s=30).status,'UNKNOWN')


if __name__=='__main__': unittest.main()
