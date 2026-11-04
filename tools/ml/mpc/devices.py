"""Pure virtual actuator transition plus research safety envelope; no real I/O."""
from __future__ import annotations
from dataclasses import dataclass
import math
from .contracts import CHANNELS, Command, SafetyEnvelope


@dataclass(frozen=True)
class DeviceState:
    applied: Command = Command()
    on_s: tuple[float, ...] = (0.,) * len(CHANNELS)
    off_s: tuple[float, ...] = (1e6,) * len(CHANNELS)

    def __post_init__(self) -> None:
        if (len(self.on_s) != len(CHANNELS) or len(self.off_s) != len(CHANNELS)
                or any(not math.isfinite(v) or v < 0 for v in self.on_s + self.off_s)):
            raise ValueError('invalid virtual device dwell history')


@dataclass(frozen=True)
class DeviceLimits:
    slew_fraction_per_s: float = 0.05
    minimum_on_s: float = 60.0
    minimum_off_s: float = 60.0
    maxima: Command = Command(1, 1, 1, 1, 1, 1, 1, 1)

    def __post_init__(self) -> None:
        if (not all(math.isfinite(v) and v >= 0 for v in
                    (self.slew_fraction_per_s, self.minimum_on_s, self.minimum_off_s))
                or self.slew_fraction_per_s > 1.):
            raise ValueError('invalid virtual device limits')


def hard_stops(*, temperature_c: float, rh_pct: float, co2_ppm: float,
               limits: SafetyEnvelope = SafetyEnvelope()) -> frozenset[str]:
    """Fail-closed stops act on previously ON devices, regardless of the new request."""
    stops = set()
    if temperature_c >= limits.temperature_max_c:
        stops.update(('heater', 'lamp_dimmer'))
    if temperature_c <= limits.temperature_min_c:
        stops.add('cooler')
    if rh_pct >= limits.rh_max_pct:
        stops.add('humidifier')
    if rh_pct <= limits.rh_min_pct:
        stops.add('dehumidifier')
    if co2_ppm >= limits.co2_max_ppm:
        stops.add('co2_doser')
    return frozenset(stops)


def safe_request(command: Command, *, temperature_c: float, rh_pct: float,
                 co2_ppm: float, limits: SafetyEnvelope = SafetyEnvelope()) -> tuple[Command, tuple[str, ...]]:
    """Hard interlocks take precedence over normal dwell; caller must apply this each cycle."""
    values = command.as_dict()
    reasons = []
    if values['heater'] > 0 and values['cooler'] > 0:
        values['heater' if temperature_c >= limits.temperature_max_c else 'cooler'] = 0.0
        reasons.append('heat_cool_conflict')
    if values['co2_doser'] > 0 and values['exhaust_fan'] > 0:
        values['co2_doser' if co2_ppm >= limits.co2_max_ppm else 'exhaust_fan'] = 0.0
        reasons.append('co2_dosing_exhaust_conflict')
    if values['humidifier'] > 0 and values['dehumidifier'] > 0:
        values['humidifier' if rh_pct >= limits.rh_max_pct else 'dehumidifier'] = 0.0
        reasons.append('humid_dehumid_conflict')
    if temperature_c >= limits.temperature_max_c:
        for n in ('heater', 'lamp_dimmer'):
            if values[n]: reasons.append('temperature_high:' + n)
            values[n] = 0.0
    if temperature_c <= limits.temperature_min_c:
        if values['cooler']: reasons.append('temperature_low:cooler')
        values['cooler'] = 0.0
    if rh_pct >= limits.rh_max_pct:
        if values['humidifier']: reasons.append('rh_high:humidifier')
        values['humidifier'] = 0.0
    if rh_pct <= limits.rh_min_pct:
        if values['dehumidifier']: reasons.append('rh_low:dehumidifier')
        values['dehumidifier'] = 0.0
    if co2_ppm >= limits.co2_max_ppm:
        if values['co2_doser']: reasons.append('co2_high:co2_doser')
        values['co2_doser'] = 0.0
    return Command(**values), tuple(reasons)


def transition(state: DeviceState, requested: Command, dt_s: float,
               limits: DeviceLimits = DeviceLimits(), faulted: frozenset[str] = frozenset(),
               emergency_stop: frozenset[str] = frozenset()) -> tuple[DeviceState, tuple[str, ...]]:
    """Advance a virtual device for dt, including slew/dwell/faults; no mutation or hardware."""
    if not 0 < dt_s <= 3600 or not math.isfinite(dt_s):
        raise ValueError('invalid device timestep')
    if set(faulted) - set(CHANNELS) or set(emergency_stop) - set(CHANNELS):
        raise ValueError('unknown actuator channel')
    vals, ons, offs, rejected = {}, [], [], []
    for i, name in enumerate(CHANNELS):
        old = getattr(state.applied, name)
        nominal = getattr(requested, name)
        desired = min(nominal, getattr(limits.maxima, name))
        if desired < nominal:
            rejected.append(name + ':max_capacity')
        hard_stop = name in emergency_stop or name in faulted
        if hard_stop:
            if name in emergency_stop and old > 0:
                rejected.append(name + ':safety_stop')
            desired = 0.0
        elif old > 0 and desired == 0 and state.on_s[i] < limits.minimum_on_s:
            desired = old
            rejected.append(name + ':minimum_on')
        elif old == 0 and desired > 0 and state.off_s[i] < limits.minimum_off_s:
            desired = 0.0
            rejected.append(name + ':minimum_off')
        if not hard_stop:
            delta = limits.slew_fraction_per_s * dt_s
            limited = max(old - delta, min(old + delta, desired))
            if abs(limited - desired) > 1e-12:
                rejected.append(name + ':slew_limit')
            desired = limited
        desired = max(0.0, min(1.0, desired))
        vals[name] = desired
        if desired > 0:
            ons.append(state.on_s[i] + dt_s if old > 0 else dt_s)
            offs.append(0.0)
        else:
            offs.append(state.off_s[i] + dt_s if old == 0 else dt_s)
            ons.append(0.0)
        if name in faulted and getattr(requested, name) > 0:
            rejected.append(name + ':failed')
    # Last-resort interlock: held-on previous device can conflict with new command.
    for a, b in (('heater', 'cooler'), ('humidifier', 'dehumidifier'),
                 ('co2_doser', 'exhaust_fan')):
        if vals[a] > 0 and vals[b] > 0:
            keep = a if getattr(state.applied, a) > 0 else b if getattr(state.applied, b) > 0 else a
            drop = b if keep == a else a
            index = CHANNELS.index(drop)
            vals[drop] = 0.0
            ons[index] = 0.0
            offs[index] = dt_s if getattr(state.applied, drop) > 0 else state.off_s[index] + dt_s
            rejected.append(drop + ':interlock')
    return DeviceState(Command(**vals), tuple(ons), tuple(offs)), tuple(rejected)


def fallback() -> Command:
    # Simulation-only safe proposal; no assertion that ventilation is always physically safe.
    return Command(exhaust_fan=0.3, circulation_fan=0.2)
