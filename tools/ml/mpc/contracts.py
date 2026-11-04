"""Research-only MPC contracts. All units explicit; parameters are NOT calibrated."""
from __future__ import annotations
from dataclasses import dataclass, fields
import math

CHANNELS = ('heater', 'cooler', 'humidifier', 'dehumidifier', 'exhaust_fan',
            'circulation_fan', 'co2_doser', 'lamp_dimmer')


@dataclass(frozen=True)
class Command:
    heater: float = 0.0
    cooler: float = 0.0
    humidifier: float = 0.0
    dehumidifier: float = 0.0
    exhaust_fan: float = 0.0
    circulation_fan: float = 0.0
    co2_doser: float = 0.0
    lamp_dimmer: float = 0.0

    def __post_init__(self) -> None:
        if any(not math.isfinite(getattr(self, n)) or not 0 <= getattr(self, n) <= 1 for n in CHANNELS):
            raise ValueError('commands must be finite fractions in [0,1]')

    def as_dict(self) -> dict[str, float]:
        return {n: getattr(self, n) for n in CHANNELS}


@dataclass(frozen=True)
class Targets:
    temperature_c: float
    rh_pct: float | None = None
    vpd_kpa: float | None = None
    co2_ppm: float = 700.0
    ppfd_umol_m2_s: float = 0.0
    dli_mol_m2: float | None = None
    day_seconds_remaining: float | None = None

    def __post_init__(self) -> None:
        if (self.rh_pct is None) == (self.vpd_kpa is None):
            raise ValueError('specify exactly one of RH or VPD')
        if not 10 <= self.temperature_c <= 40 or not 300 <= self.co2_ppm <= 1800:
            raise ValueError('target outside research envelope')
        if self.rh_pct is not None and not 20 <= self.rh_pct <= 90:
            raise ValueError('invalid RH target')
        if self.vpd_kpa is not None and not 0.1 <= self.vpd_kpa <= 3:
            raise ValueError('invalid VPD target')
        if not 0 <= self.ppfd_umol_m2_s <= 2000:
            raise ValueError('invalid PPFD target')
        if self.dli_mol_m2 is not None and not 0 <= self.dli_mol_m2 <= 70:
            raise ValueError('invalid DLI target')
        if self.day_seconds_remaining is not None and self.day_seconds_remaining <= 0:
            raise ValueError('invalid remaining photoperiod')


@dataclass(frozen=True)
class SafetyEnvelope:
    temperature_min_c: float = 12.0
    temperature_max_c: float = 36.0
    rh_min_pct: float = 20.0
    rh_max_pct: float = 90.0
    co2_max_ppm: float = 1800.0
    max_sensor_age_s: float = 180.0

    def __post_init__(self) -> None:
        if (not all(math.isfinite(v) for v in (
                self.temperature_min_c, self.temperature_max_c,
                self.rh_min_pct, self.rh_max_pct,
                self.co2_max_ppm, self.max_sensor_age_s))
                or not -40 <= self.temperature_min_c < self.temperature_max_c <= 80
                or not 0 <= self.rh_min_pct < self.rh_max_pct <= 100
                or not 300 <= self.co2_max_ppm <= 100000
                or self.max_sensor_age_s <= 0):
            raise ValueError('invalid research safety envelope')


@dataclass(frozen=True)
class MPCConfig:
    step_s: float = 60.0
    horizon_steps: int = 3
    beam_width: int = 2
    max_evaluations: int = 60
    time_limit_s: float = 2.0
    seed: int = 1729
    energy_weight: float = 0.02
    switching_weight: float = 0.025

    def __post_init__(self) -> None:
        if not (0 < self.step_s <= 3600 and 1 <= self.horizon_steps <= 12
                and 1 <= self.beam_width <= 8 and 1 <= self.max_evaluations <= 2000
                and 0 < self.time_limit_s <= 120 and self.energy_weight >= 0
                and self.switching_weight >= 0):
            raise ValueError('invalid bounded MPC configuration')


@dataclass(frozen=True)
class Decision:
    status: str  # PASS / FAIL / BLOCKED / UNKNOWN (evidence), not claim of physics fidelity
    requested: Command
    reason: str
    evaluations: int
    predicted_cost: float | None
    predictions: tuple[dict, ...] = ()
    uncertainty: str = 'synthetic_uncalibrated'
