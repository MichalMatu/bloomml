"""Passive ESP32-S3 shadow research contract; absolutely no execution or I/O.

This function *interprets already supplied* hypothetical Rule/Supervisor records.
It does not call Rule, firmware, GPIO, network, storage or physical transports.
Missing execution evidence is NEVER converted into a successful acknowledgement.
"""
from __future__ import annotations

from dataclasses import dataclass
import math
import re

from .contracts import Command


@dataclass(frozen=True)
class ShadowProposal:
    proposal_id: str
    source_commit: str
    observed_at_s: float
    sensor_valid: bool
    sensor_age_s: float
    command: Command
    confidence: float | None = None
    provenance: str = 'synthetic_uncalibrated'

    def __post_init__(self) -> None:
        if not self.proposal_id or len(self.proposal_id) > 100:
            raise ValueError('proposal id required')
        if not math.isfinite(self.observed_at_s) or not math.isfinite(self.sensor_age_s):
            raise ValueError('invalid observation timing')
        if self.confidence is not None and (not math.isfinite(self.confidence)
                                            or not 0 <= self.confidence <= 1):
            raise ValueError('invalid confidence')


@dataclass(frozen=True)
class RuleVerdict:
    proposal_id: str
    accepted: bool
    reason: str


@dataclass(frozen=True)
class SupervisorFeedback:
    proposal_id: str
    status: str  # REJECTED / NOT_ATTEMPTED / SENT / SIMULATED_ACK / TIMEOUT
    reason: str = ''
    updated_at_s: float | None = None


@dataclass(frozen=True)
class ShadowTrace:
    status: str  # PASS/FAIL/BLOCKED/UNKNOWN for shadow evidence, NOT physical safety
    proposal_id: str
    reason: str
    proposed: Command
    rule_accepted: bool | None
    supervisor_status: str
    execution_acknowledged: bool  # ALWAYS false: hypothetical offline record isn't physical ACK
    fallback: Command
    simulation_only: bool = True


def evaluate_shadow(proposal: ShadowProposal, *, now_s: float,
                    rule_verdict: RuleVerdict | None = None,
                    supervisor_feedback: SupervisorFeedback | None = None,
                    max_sensor_age_s: float = 180., timeout_s: float = 120.) -> ShadowTrace:
    """Inspect independent offline evidence in order; never issue a command.

    `SIMULATED_ACK` means a *fake* reports completion; even then `execution_acknowledged`
    stays False because no physical acknowledgement exists. No production wiring.
    """
    if not math.isfinite(now_s) or not 0 < timeout_s or not 0 < max_sensor_age_s:
        raise ValueError('invalid offline evaluation clock')
    safe = Command()  # no claim about real safe fallback; purely a diagnostic plan
    def result(status, reason, rule=None, supervisor='NOT_ATTEMPTED'):
        return ShadowTrace(status, proposal.proposal_id, reason, proposal.command,
                           rule, supervisor, False, safe)

    if (proposal.observed_at_s > now_s or proposal.sensor_age_s < 0 or
            now_s - proposal.observed_at_s > max_sensor_age_s or
            proposal.sensor_age_s > max_sensor_age_s or not proposal.sensor_valid):
        return result('BLOCKED', 'invalid_or_stale_sensor')
    if not re.fullmatch(r'[0-9a-f]{40}', proposal.source_commit):
        return result('UNKNOWN', 'missing_or_invalid_source_sha')
    if rule_verdict is None:
        return result('BLOCKED', 'missing_rule_verdict')
    if rule_verdict.proposal_id != proposal.proposal_id:
        return result('FAIL', 'rule_proposal_id_mismatch')
    if not rule_verdict.accepted:
        return result('BLOCKED', 'rule_rejected:' + rule_verdict.reason, False, 'NOT_ATTEMPTED')
    if supervisor_feedback is None:
        return result('BLOCKED', 'execution_evidence_missing', True)
    if supervisor_feedback.proposal_id != proposal.proposal_id:
        return result('FAIL', 'supervisor_proposal_id_mismatch', True)
    if supervisor_feedback.status not in ('REJECTED','NOT_ATTEMPTED','SENT','SIMULATED_ACK','TIMEOUT'):
        return result('FAIL', 'invalid_supervisor_status', True)
    if supervisor_feedback.updated_at_s is None or not math.isfinite(supervisor_feedback.updated_at_s):
        return result('UNKNOWN', 'supervisor_time_unknown', True, supervisor_feedback.status)
    if (supervisor_feedback.updated_at_s < proposal.observed_at_s or
            supervisor_feedback.updated_at_s > now_s):
        return result('FAIL', 'supervisor_time_invalid', True, supervisor_feedback.status)
    if (supervisor_feedback.status == 'TIMEOUT' or
            supervisor_feedback.updated_at_s - proposal.observed_at_s > timeout_s):
        return result('BLOCKED', 'supervisor_timeout', True, supervisor_feedback.status)
    if supervisor_feedback.status == 'REJECTED':
        return result('BLOCKED', 'supervisor_rejected:' + supervisor_feedback.reason,
                      True, supervisor_feedback.status)
    if supervisor_feedback.status in ('SENT','NOT_ATTEMPTED'):
        return result('BLOCKED', 'no_execution_ack', True, supervisor_feedback.status)
    return result('PASS', 'simulated_ack_only', True, 'SIMULATED_ACK')
