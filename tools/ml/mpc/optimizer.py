"""Bounded, seed-stable beam-search MPC over the existing reference physics.

No hardware, no firmware. Uses the exact GrowboxThermodynamics.step implementation
as the predictive transition (no competing physical surrogate).
"""
from __future__ import annotations
from dataclasses import dataclass, replace
import math
import random
import time
from .contracts import CHANNELS, Command, Decision, MPCConfig, SafetyEnvelope, Targets
from .devices import DeviceLimits, DeviceState, fallback, safe_request, transition


@dataclass
class _Node:
    state: object
    devices: DeviceState
    cost: float
    first: Command
    predictions: tuple[dict, ...]


def _measure(physics, state, forcing, command):
    from tools.ml.physics.growbox_dynamics import GrowboxThermodynamics
    model = GrowboxThermodynamics(physics, state)
    observation = model.observe(replace(forcing, **command.as_dict()))
    return model, observation


def _cost(state, obs, targets: Targets, command: Command, old: Command,
          cfg: MPCConfig, physics, envelope: SafetyEnvelope) -> float:
    # Dimensionless squared residuals, per-variable engineering tolerances.
    result = ((state.air_temperature_c - targets.temperature_c) / 2.0) ** 2
    humidity = (obs.rh_pct - targets.rh_pct) / 10 if targets.rh_pct is not None else (
        (obs.air_vpd_kpa - targets.vpd_kpa) / 0.3)
    result += humidity ** 2
    result += ((state.co2_ppm - targets.co2_ppm) / 200.0) ** 2
    result += ((obs.ppfd_umol_m2_s - targets.ppfd_umol_m2_s) / 150.0) ** 2
    if targets.dli_mol_m2 is not None and targets.day_seconds_remaining is not None:
        # DLI pacing is a soft objective; cannot guarantee feasibility.
        demand = max(0., targets.dli_mol_m2 - state.cumulative_dli_mol_m2)
        possible = obs.ppfd_umol_m2_s * targets.day_seconds_remaining * 1e-6
        result += ((max(0., demand - possible)) / 5.0) ** 2
    eq = physics.equipment
    light = physics.lighting
    # Electrical proxy: cooling COP / humidifier power unavailable => assumptions, not measured kWh.
    watts = (eq.heater_power_w * command.heater + eq.cooler_capacity_w * command.cooler
             + light.rated_power_w * command.lamp_dimmer + 40 * command.humidifier
             + 70 * command.dehumidifier + 25 * command.co2_doser
             + 35 * command.exhaust_fan + 5 * command.circulation_fan)
    result += cfg.energy_weight * watts / 100.0
    result += cfg.switching_weight * sum((getattr(command, n) > 0) != (getattr(old, n) > 0)
                                          for n in CHANNELS)
    # Envelope violations should dominate ordinary tracking costs.
    result += 1e3 * (max(0., envelope.temperature_min_c - state.air_temperature_c) ** 2
                     + max(0., state.air_temperature_c - envelope.temperature_max_c) ** 2
                     + max(0., obs.rh_pct - envelope.rh_max_pct) ** 2
                     + max(0., envelope.rh_min_pct - obs.rh_pct) ** 2
                     + max(0., state.co2_ppm - envelope.co2_max_ppm) ** 2)
    return result


def _candidates(state, obs, targets, prior: Command, physics, seed: int) -> tuple[Command, ...]:
    eq, light, geom = physics.equipment, physics.lighting, physics.geometry
    delta_t = targets.temperature_c - state.air_temperature_c
    delta_h = (targets.rh_pct - obs.rh_pct) if targets.rh_pct is not None else (
        (obs.air_vpd_kpa - targets.vpd_kpa) * 25)
    delta_co2 = targets.co2_ppm - state.co2_ppm
    ideal_lamp = targets.ppfd_umol_m2_s * geom.canopy_area_m2 / max(
        1e-9, light.rated_power_w * light.photon_efficacy_umol_j * light.canopy_capture_fraction)
    ideal = Command(
        heater=min(1., max(0., delta_t / 3)), cooler=min(1., max(0., -delta_t / 3)),
        humidifier=min(1., max(0., delta_h / 15)), dehumidifier=min(1., max(0., -delta_h / 15)),
        co2_doser=min(1., max(0., delta_co2 / 300)),
        exhaust_fan=min(1., max(0., -delta_co2 / 400)), circulation_fan=0.25,
        lamp_dimmer=min(1., max(0., ideal_lamp)))
    choices = [ideal, prior, Command(), fallback()]
    # Individual deviations allow optimizer to discover couplings (ventilation vs heater/RH/CO2).
    rng = random.Random(seed)
    names = list(CHANNELS)
    rng.shuffle(names)
    for n in names:
        v = getattr(ideal, n)
        choices.append(replace(ideal, **{n: 0. if v >= 0.5 else 1.}))
    return tuple(dict.fromkeys(choices))


def optimize(physics, state, forcing, targets: Targets, devices: DeviceState,
             *, cfg: MPCConfig = MPCConfig(), envelope: SafetyEnvelope = SafetyEnvelope(),
             device_limits: DeviceLimits = DeviceLimits(), sensor_age_s: float = 0.,
             faulted: frozenset[str] = frozenset()) -> Decision:
    if not math.isfinite(sensor_age_s) or sensor_age_s < 0 or sensor_age_s > envelope.max_sensor_age_s:
        return Decision('BLOCKED', fallback(), 'stale_or_invalid_sensor', 0, None)
    deadline = time.monotonic() + cfg.time_limit_s
    evaluations = 0
    try:
        beam = [_Node(state, devices, 0., fallback(), ())]
        last_completed = None
        for depth in range(cfg.horizon_steps):
            if time.monotonic() >= deadline:
                return Decision('BLOCKED', fallback(), 'optimizer_timeout', evaluations, None)
            children = []
            for node_index, node in enumerate(beam):
                _, observation = _measure(physics, node.state, forcing, node.devices.applied)
                for desired in _candidates(node.state, observation, targets, node.devices.applied,
                                           physics, cfg.seed + 101 * depth + node_index):
                    if evaluations >= cfg.max_evaluations or time.monotonic() >= deadline:
                        break
                    safe, _ = safe_request(desired, temperature_c=node.state.air_temperature_c,
                                           rh_pct=observation.rh_pct, co2_ppm=node.state.co2_ppm,
                                           limits=envelope)
                    stop = frozenset(n for n in CHANNELS if getattr(safe, n) == 0 and
                                     getattr(desired, n) > 0)
                    new_devices, _ = transition(node.devices, safe, cfg.step_s, device_limits,
                                                 faulted=faulted, emergency_stop=stop)
                    model, _ = _measure(physics, node.state, forcing, new_devices.applied)
                    new_state = model.step(replace(forcing, **new_devices.applied.as_dict()), cfg.step_s)
                    new_obs = model.observe(replace(forcing, **new_devices.applied.as_dict()))
                    evaluations += 1
                    score = _cost(new_state, new_obs, targets, new_devices.applied,
                                  node.devices.applied, cfg, physics, envelope)
                    if not math.isfinite(score):
                        continue
                    prediction = {'step': depth + 1, 'temperature_c': new_state.air_temperature_c,
                                  'rh_pct': new_obs.rh_pct, 'co2_ppm': new_state.co2_ppm,
                                  'ppfd_umol_m2_s': new_obs.ppfd_umol_m2_s,
                                  'dli_mol_m2': new_state.cumulative_dli_mol_m2}
                    children.append(_Node(new_state, new_devices, node.cost + score,
                                          safe if depth == 0 else node.first,
                                          node.predictions + (prediction,)))
                if evaluations >= cfg.max_evaluations or time.monotonic() >= deadline:
                    break
            if not children:
                break
            children.sort(key=lambda n: n.cost)
            beam = children[:cfg.beam_width]
            last_completed = beam[0]
            if evaluations >= cfg.max_evaluations or time.monotonic() >= deadline:
                break
        if time.monotonic() >= deadline:
            return Decision('BLOCKED', fallback(), 'optimizer_timeout', evaluations, None)
        if last_completed is None:
            return Decision('BLOCKED', fallback(), 'optimizer_budget_exhausted', evaluations, None)
        return Decision('PASS', last_completed.first, 'bounded_search', evaluations,
                        last_completed.cost, last_completed.predictions)
    except (ValueError, OverflowError, ZeroDivisionError) as exc:
        return Decision('FAIL', fallback(), 'physics_prediction_failed:' + type(exc).__name__, evaluations, None)
