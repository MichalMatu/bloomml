"""Offline closed-loop execution and like-for-like threshold/hysteresis baselines."""
from __future__ import annotations
from dataclasses import dataclass, replace
import math
from .contracts import Command, MPCConfig, SafetyEnvelope, Targets
from .devices import DeviceLimits, DeviceState, fallback, hard_stops, safe_request, transition
from .optimizer import optimize


def threshold(state, observation, targets: Targets, *, max_ppfd: float, deadband: float = 0.,
              prior: Command = Command()) -> Command:
    # Stateful Schmitt trigger only when deadband > 0; threshold baseline has no memory.
    def low(value: float, goal: float, was_on: bool) -> float:
        return float(value < goal + deadband if was_on else value < goal - deadband)

    def high(value: float, goal: float, was_on: bool) -> float:
        return float(value > goal - deadband if was_on else value > goal + deadband)

    dh = (targets.rh_pct - observation.rh_pct) if targets.rh_pct is not None else (
        (observation.air_vpd_kpa - targets.vpd_kpa) * 20)
    return Command(heater=low(state.air_temperature_c, targets.temperature_c, prior.heater > 0),
                   cooler=high(state.air_temperature_c, targets.temperature_c, prior.cooler > 0),
                   humidifier=high(dh, 5., prior.humidifier > 0),
                   dehumidifier=low(dh, -5., prior.dehumidifier > 0),
                   co2_doser=low(state.co2_ppm, targets.co2_ppm - 70., prior.co2_doser > 0),
                   exhaust_fan=.4 * high(state.co2_ppm, targets.co2_ppm + 100., prior.exhaust_fan > 0),
                   circulation_fan=.25,
                   lamp_dimmer=min(1., max(0., targets.ppfd_umol_m2_s / max(1e-9, max_ppfd))))


@dataclass(frozen=True)
class EpisodeMetrics:
    status: str
    steps: int
    temp_mae_c: float
    temp_rmse_c: float
    rh_mae_pp: float | None
    rh_rmse_pp: float | None
    vpd_mae_kpa: float | None
    co2_mae_ppm: float
    co2_rmse_ppm: float
    ppfd_mae_umol_m2_s: float
    outside_tolerance_steps: int
    safety_violations: int
    switches: int
    energy_proxy_wh: float
    water_residual_max_kg: float
    fallback_steps: int
    failures: tuple[str, ...]


def run_episode(physics, initial_state, forcing_at, targets_at, *, steps: int,
                cfg: MPCConfig = MPCConfig(), policy: str = 'mpc',
                faulted_at=None, missing_at=None, envelope: SafetyEnvelope = SafetyEnvelope(),
                limits: DeviceLimits = DeviceLimits(), source_commit: str | None = None):
    """Callable schedules get virtual time seconds; never connect to physical transport."""
    from tools.ml.physics.growbox_dynamics import GrowboxThermodynamics
    if policy not in ('mpc', 'threshold', 'hysteresis') or not 1 <= steps <= 100_000:
        raise ValueError('invalid episode')
    plant = GrowboxThermodynamics(physics, initial_state)
    devices = DeviceState()
    rows = []
    errors_t, errors_h, errors_vpd, errors_co2, errors_ppfd, energy = [], [], [], [], [], 0.
    switches = outside = violated = fallback_count = 0
    failures = []
    max_water = 0.
    for tick in range(steps):
        now = tick * cfg.step_s
        forcing = forcing_at(now)
        targets = targets_at(now)
        obs = plant.observe(replace(forcing, **devices.applied.as_dict()))
        missing_age = missing_at(now) if missing_at else 0.
        if policy == 'mpc':
            decision = optimize(physics, plant.state, forcing, targets, devices,
                                cfg=cfg, envelope=envelope, device_limits=limits,
                                sensor_age_s=missing_age,
                                faulted=faulted_at(now) if faulted_at else frozenset())
            request, status, reason = decision.requested, decision.status, decision.reason
            predictions, evaluations, predicted_cost = (decision.predictions, decision.evaluations,
                                                       decision.predicted_cost)
            uncertainty = decision.uncertainty
        else:
            request = fallback() if missing_age > envelope.max_sensor_age_s else threshold(
                plant.state, obs, targets,
                max_ppfd=(physics.lighting.rated_power_w * physics.lighting.photon_efficacy_umol_j
                          * physics.lighting.canopy_capture_fraction / physics.geometry.canopy_area_m2),
                deadband=1.0 if policy == 'hysteresis' else 0.,
                prior=devices.applied if policy == 'hysteresis' else Command())
            status, reason = ('BLOCKED', 'stale_sensor') if missing_age > envelope.max_sensor_age_s else ('PASS', policy)
            predictions, evaluations, predicted_cost, uncertainty = (), 0, None, 'baseline_no_forecast'
        if status != 'PASS':
            fallback_count += 1
            failures.append(f'{tick}:{reason}')
        approved, rejects = safe_request(request, temperature_c=plant.state.air_temperature_c,
                                         rh_pct=obs.rh_pct, co2_ppm=plant.state.co2_ppm,
                                         limits=envelope)
        stop = hard_stops(temperature_c=plant.state.air_temperature_c,
                          rh_pct=obs.rh_pct, co2_ppm=plant.state.co2_ppm,
                          limits=envelope) | frozenset(
            n for n in request.as_dict() if getattr(request, n) > 0 and getattr(approved, n) == 0)
        virtual_faults = faulted_at(now) if faulted_at else frozenset()
        previous = devices.applied
        devices, actuator_rejects = transition(devices, approved, cfg.step_s, limits,
                             faulted=virtual_faults, emergency_stop=stop)
        switches += sum((getattr(previous, n) > 0) != (getattr(devices.applied, n) > 0)
                        for n in previous.as_dict())
        plant.step(replace(forcing, **devices.applied.as_dict()), cfg.step_s)
        next_obs = plant.observe(replace(forcing, **devices.applied.as_dict()))
        te = plant.state.air_temperature_c - targets.temperature_c
        he = next_obs.rh_pct - (targets.rh_pct if targets.rh_pct is not None else next_obs.rh_pct)
        ve = next_obs.air_vpd_kpa - targets.vpd_kpa if targets.vpd_kpa is not None else None
        ce = plant.state.co2_ppm - targets.co2_ppm
        pe = next_obs.ppfd_umol_m2_s - targets.ppfd_umol_m2_s
        errors_t.append(te)
        if targets.rh_pct is not None: errors_h.append(he)
        if ve is not None: errors_vpd.append(ve)
        errors_co2.append(ce); errors_ppfd.append(pe)
        if (abs(te) > 2 or (targets.rh_pct is not None and abs(he) > 10)
                or (ve is not None and abs(ve) > .3) or abs(ce) > 200 or abs(pe) > 150):
            outside += 1
        if not (envelope.temperature_min_c <= plant.state.air_temperature_c <= envelope.temperature_max_c
                and envelope.rh_min_pct <= next_obs.rh_pct <= envelope.rh_max_pct
                and plant.state.co2_ppm <= envelope.co2_max_ppm):
            violated += 1
        power_proxy = (physics.equipment.heater_power_w * devices.applied.heater
                       + physics.equipment.cooler_capacity_w * devices.applied.cooler
                       + physics.lighting.rated_power_w * devices.applied.lamp_dimmer
                       + 35 * devices.applied.exhaust_fan + 70 * devices.applied.dehumidifier
                       + 40 * devices.applied.humidifier + 25 * devices.applied.co2_doser)
        energy += power_proxy * cfg.step_s / 3600
        max_water = max(max_water, abs(plant.water_balance_residual_kg()))
        first_forecast = predictions[0] if predictions else None
        one_step_error = ({'temperature_c': plant.state.air_temperature_c - first_forecast['temperature_c'],
                           'rh_pct': next_obs.rh_pct - first_forecast['rh_pct'],
                           'co2_ppm': plant.state.co2_ppm - first_forecast['co2_ppm'],
                           'ppfd_umol_m2_s': next_obs.ppfd_umol_m2_s - first_forecast['ppfd_umol_m2_s']}
                          if first_forecast else None)
        rows.append({'tick': tick, 'time_s': now, 'targets': vars(targets),
                     'state': {'temperature_c': plant.state.air_temperature_c,
                               'rh_pct': next_obs.rh_pct, 'co2_ppm': plant.state.co2_ppm,
                               'ppfd_umol_m2_s': next_obs.ppfd_umol_m2_s},
                     'proposed': request.as_dict(), 'applied': devices.applied.as_dict(),
                     'rejected': list(rejects + actuator_rejects), 'status': status,
                     'reason': reason, 'predictions': predictions,
                     'prediction_uncertainty': uncertainty, 'prediction_cost': predicted_cost,
                     'one_step_prediction_error': one_step_error,
                     'optimizer_evaluations': evaluations,
                     'virtual_execution_status': 'faulted_simulation' if virtual_faults else 'simulated_applied',
                     'virtual_device_faults': sorted(virtual_faults),
                     'water_residual_kg': plant.water_balance_residual_kg(),
                     'water_inventory_kg': plant.state.total_water_kg(),
                     'cumulative_external_water_kg': plant.state.cumulative_net_external_water_kg,
                     'cumulative_added_water_kg': plant.state.cumulative_added_water_kg,
                     'energy_proxy_wh_cumulative': energy,
                     'origin': 'synthetic_uncalibrated', 'source_commit': source_commit or 'UNKNOWN',
                     'seed': cfg.seed, 'sensor_age_s': missing_age,
                     'physical_validation_with_real_data': False})
    def mae(x): return sum(map(abs, x)) / len(x) if x else None
    def rmse(x): return math.sqrt(sum(v*v for v in x)/len(x)) if x else None
    import re
    sha_ok = bool(source_commit and re.fullmatch(r'[0-9a-f]{40}', source_commit))
    status = ('FAIL' if failures or violated or outside > int(0.2 * steps)
              else 'UNKNOWN' if not sha_ok else 'PASS')
    metrics = EpisodeMetrics(status, steps, mae(errors_t), rmse(errors_t),
        mae(errors_h), rmse(errors_h), mae(errors_vpd), mae(errors_co2),
        rmse(errors_co2), mae(errors_ppfd), outside, violated, switches,
        energy, max_water, fallback_count, tuple(failures))
    return metrics, rows
