"""Research-only vectorised PyTorch implementation of the v2 growbox balances.

An explicit second numerical implementation: the scalar GrowboxThermodynamics
remains the oracle. No firmware or actuator authority; no implicit CPU fallback.
One tensor element is one independent chamber scenario (no cross-scenario flux).

PyTorch is imported only when this optional research module is used.
"""

from __future__ import annotations

import math
from dataclasses import dataclass, fields
from typing import Sequence

import torch

from .growbox_dynamics import (
    GrowboxForcing, GrowboxPhysics, GrowboxState, GrowboxThermodynamics,
)
from .growbox_psychrometrics import R_DRY_AIR, R_WATER_VAPOR, LATENT_HEAT_VAPORIZATION
from .growbox_convection import _h_w_m2_k, REFERENCE_SPEED_M_S, REFERENCE_DELTA_T_K

_STATE_NAMES = tuple(f.name for f in fields(GrowboxState))
_FLOAT_TYPES = (torch.float32, torch.float64)


@dataclass(frozen=True)
class BatchResult:
    """Host-side result after a single device-to-host transfer per state field."""

    states: tuple[GrowboxState, ...]
    water_residuals_kg: tuple[float, ...]
    device: str
    dtype: str
    scenarios: int
    ticks: int
    substeps_per_tick: tuple[int, ...]


def _saturation_density(t: torch.Tensor) -> torch.Tensor:
    return 610.94 * torch.exp(17.625 * t / (t + 243.04)) / (R_WATER_VAPOR * (273.15 + t))


def _saturation_pressure(t: torch.Tensor) -> torch.Tensor:
    return 610.94 * torch.exp(17.625 * t / (t + 243.04))


def _density(t: torch.Tensor, pressure: torch.Tensor, vapor_pressure: torch.Tensor) -> torch.Tensor:
    absolute = t + 273.15
    return ((pressure - vapor_pressure) / (R_DRY_AIR * absolute)
            + vapor_pressure / (R_WATER_VAPOR * absolute))


def _convection_factor(speed: torch.Tensor, delta: torch.Tensor, length: float) -> torch.Tensor:
    # These are precisely the CPU reference correlations; no CFD is claimed.
    re = speed * length / 1.56e-5
    pr = 0.71 ** (1.0 / 3.0)
    nu = torch.where(re <= 500_000,
                     0.664 * torch.sqrt(re) * pr,
                     torch.clamp(0.037 * re ** 0.8 - 871.0, min=0) * pr)
    forced = nu * 0.0263 / length
    natural = 1.31 * (torch.abs(delta) / length) ** (1.0 / 3.0)
    actual = (forced ** 3 + natural ** 3) ** (1.0 / 3.0)
    reference = _h_w_m2_k(REFERENCE_SPEED_M_S, REFERENCE_DELTA_T_K, length)
    return torch.clamp(actual / reference, min=0, max=4)


def _stack(items: Sequence, name: str, device: torch.device, dtype: torch.dtype) -> torch.Tensor:
    return torch.tensor([getattr(item, name) for item in items], device=device, dtype=dtype)


def _optional(items: Sequence, name: str, device: torch.device, dtype: torch.dtype) -> torch.Tensor:
    # NaN represents omitted optional *input* only; we never advance NaN state.
    return torch.tensor([float('nan') if getattr(x, name) is None else getattr(x, name)
                         for x in items], device=device, dtype=dtype)


def _parameter_fields(parameters: Sequence[GrowboxPhysics], section: str,
                      device: torch.device, dtype: torch.dtype) -> dict[str, torch.Tensor]:
    subset = [getattr(p, section) for p in parameters]
    return {f.name: _stack(subset, f.name, device, dtype) for f in fields(subset[0])}


def _substep(s: dict[str, torch.Tensor], forcing: dict[str, torch.Tensor],
             geom: dict[str, torch.Tensor], equip: dict[str, torch.Tensor],
             bio: dict[str, torch.Tensor], light: dict[str, torch.Tensor],
             reference_density: torch.Tensor, h: torch.Tensor) -> dict[str, torch.Tensor]:
    """All fluxes operate on N-vectors; no host roundtrip in the integrator."""
    g, e, b, l, f = geom, equip, bio, light, forcing
    v = g['air_volume_m3']
    t_air, t_wall, t_leaf, t_sub = (s[k] for k in (
        'air_temperature_c', 'wall_temperature_c', 'leaf_temperature_c', 'substrate_temperature_c'))
    exhaust = torch.where(torch.isfinite(f['measured_exhaust_m3_h']),
                          f['measured_exhaust_m3_h'], e['exhaust_max_m3_h'] * f['exhaust_fan'])
    speed_est = torch.sqrt(e['background_air_speed_m_s'] ** 2
                           + (f['exhaust_fan'] * e['exhaust_induced_air_speed_m_s']) ** 2
                           + (f['circulation_fan'] * e['circulation_reference_air_speed_m_s']) ** 2)
    speed = torch.where(torch.isfinite(f['measured_air_speed_m_s']), f['measured_air_speed_m_s'], speed_est)
    leaf_factor = _convection_factor(speed, t_leaf - t_air, 0.10)
    wall_factor = _convection_factor(speed, t_wall - t_air, 0.80)
    substrate_factor = _convection_factor(speed, t_sub - t_air, 0.40)

    lamp_w = torch.where(torch.isfinite(f['lamp_measured_electrical_w']),
                         f['lamp_measured_electrical_w'], l['rated_power_w'] * f['lamp_dimmer'])
    ppfd_est = lamp_w * l['photon_efficacy_umol_j'] * l['canopy_capture_fraction'] / g['canopy_area_m2']
    ppfd = torch.where(torch.isfinite(f['canopy_measured_ppfd_umol_m2_s']),
                       f['canopy_measured_ppfd_umol_m2_s'], ppfd_est) + f['solar_measured_ppfd_umol_m2_s']
    lamp_box = lamp_w * l['in_box_fraction']
    solar_box = f['solar_irradiance_w_m2'] * l['solar_window_area_m2']
    light_air = lamp_box * l['air_fraction']
    light_leaf = lamp_box * l['leaf_fraction'] + solar_box * l['solar_canopy_fraction']
    light_wall = (lamp_box * l['wall_fraction'] + solar_box * (1.0 - l['solar_canopy_fraction'])
                  + f['solar_irradiance_w_m2'] * g['external_solar_area_m2'] * g['external_solar_absorptivity'])

    vapor_density = s['air_vapor_kg'] / v
    partial_pa = vapor_density * R_WATER_VAPOR * (t_air + 273.15)
    density = _density(t_air, f['atmospheric_pressure_pa'], partial_pa)
    air_capacity = reference_density * v * 1005.0 * g['air_heat_capacity_multiplier']
    outside_vapor = _saturation_density(f['outside_temperature_c']) * f['outside_relative_humidity_pct'] / 100.0
    q = exhaust / 3600.0 + e['infiltration_ach'] * v / 3600.0
    water_vent = q * (outside_vapor - vapor_density) * h
    heat_vent = density * 1005.0 * q * (f['outside_temperature_c'] - t_air)

    wall_flux = g['wall_vapor_transfer_m_s'] * wall_factor * g['wall_surface_area_m2'] * (vapor_density - _saturation_density(t_wall)) * h
    wall_flux = torch.where(wall_flux > 0, torch.minimum(wall_flux, s['air_vapor_kg']),
                            -torch.minimum(-wall_flux, s['wall_condensate_kg']))
    leaf_flux = g['leaf_vapor_transfer_m_s'] * leaf_factor * g['canopy_area_m2'] * (vapor_density - _saturation_density(t_leaf)) * h
    leaf_flux = torch.where(leaf_flux > 0, torch.minimum(leaf_flux, s['air_vapor_kg']),
                            -torch.minimum(-leaf_flux, s['leaf_condensate_kg']))

    soil_rh = torch.clamp(s['substrate_water_kg'] / b['substrate_water_capacity_kg'] * 2.5, min=0, max=1)
    soil_deficit = torch.clamp(_saturation_density(t_sub) * soil_rh - vapor_density, min=0)
    soil_evap = torch.minimum(s['substrate_water_kg'],
                              g['substrate_evaporation_kg_m2_s'] * g['bare_substrate_area_m2']
                              * soil_deficit / 0.01 * substrate_factor * h)
    leaf_vpd = torch.clamp((_saturation_pressure(t_leaf) - partial_pa) / 1000, min=0)
    light_response = ppfd / (ppfd + b['light_half_saturation_ppfd'])
    stomata = b['transpiration_dark_fraction'] + (1.0 - b['transpiration_dark_fraction']) * light_response
    vpd_response = (leaf_vpd / (leaf_vpd + b['vpd_half_saturation_kpa'])
                    / (1 + (leaf_vpd / b['stomatal_closure_vpd_kpa']) ** 2))
    soil_response = torch.clamp(s['substrate_water_kg'] / (0.3 * b['substrate_water_capacity_kg']), min=0, max=1)
    transp = torch.minimum(torch.clamp(s['substrate_water_kg'] - soil_evap, min=0),
                           b['maximum_transpiration_g_h'] / 3_600_000.0 * stomata * vpd_response
                           * soil_response * torch.clamp(leaf_factor, min=0) ** 0.25 * h)

    humidifier = e['humidifier_output_g_h'] / 3_600_000.0 * f['humidifier'] * h
    dehumidifier = torch.minimum(s['air_vapor_kg'], e['dehumidifier_removal_g_h'] / 3_600_000.0 * f['dehumidifier'] * h)
    coil_t = t_air - e['cooler_coil_depression_c']
    coil_sat = _saturation_density(torch.clamp(coil_t, min=-40))
    coil_condensate = torch.where((f['cooler'] > 0) & (coil_t >= -40),
                                  torch.minimum(f['cooler'] * e['cooler_coil_condensation_kg_s_k']
                                                * torch.clamp(vapor_density - coil_sat, min=0)
                                                / torch.clamp(coil_sat, min=1.0e-6) * h, s['air_vapor_kg']),
                                  torch.zeros_like(h))
    available = torch.clamp(s['air_vapor_kg'] + water_vent + humidifier + transp + soil_evap, min=0)
    total_out = (torch.clamp(wall_flux, min=0) + torch.clamp(leaf_flux, min=0)
                 + dehumidifier + coil_condensate)
    scale = torch.where(total_out > available, available / torch.clamp(total_out, min=1.e-30),
                        torch.ones_like(total_out))
    wall_flux = torch.where(total_out > available, torch.clamp(wall_flux, min=0) * scale, wall_flux)
    leaf_flux = torch.where(total_out > available, torch.clamp(leaf_flux, min=0) * scale, leaf_flux)
    dehumidifier = dehumidifier * scale
    coil_condensate = coil_condensate * scale
    vapor_new = torch.clamp(s['air_vapor_kg'] + water_vent + humidifier + transp + soil_evap
                            - wall_flux - leaf_flux - dehumidifier - coil_condensate, min=0)
    fog = s['fog_condensate_kg']
    vapor_capacity = _saturation_density(t_air) * v
    revap = torch.where((vapor_new < vapor_capacity) & (fog > 0),
                        torch.minimum(torch.minimum(fog, vapor_capacity - vapor_new), 0.02 * vapor_capacity),
                        torch.zeros_like(fog))
    vapor_new = vapor_new + revap
    fog = fog - revap

    leaf_air = g['leaf_air_conductance_w_k'] * leaf_factor * (t_leaf - t_air)
    wall_air = g['wall_air_conductance_w_k'] * wall_factor * (t_wall - t_air)
    substrate_air = g['substrate_air_conductance_w_k'] * substrate_factor * (t_sub - t_air)
    cooler_power = e['cooler_capacity_w'] * f['cooler']
    latent_coil = torch.minimum(cooler_power, coil_condensate / h * LATENT_HEAT_VAPORIZATION)
    dt_air = ((e['heater_power_w'] * f['heater'] + light_air
               + e['fan_electrical_heat_w'] * f['exhaust_fan']
               + e['circulation_fan_heat_w'] * f['circulation_fan']
               + e['dehumidifier_heat_w'] * f['dehumidifier']
               + dehumidifier / h * LATENT_HEAT_VAPORIZATION * e['dehumidifier_latent_return_fraction']
               - cooler_power + latent_coil
               - humidifier / h * LATENT_HEAT_VAPORIZATION * e['humidifier_air_cooling_fraction']
               - revap / h * LATENT_HEAT_VAPORIZATION
               + heat_vent + leaf_air + wall_air + substrate_air) * h / air_capacity)
    dt_leaf = ((light_leaf - leaf_air + leaf_flux / h * LATENT_HEAT_VAPORIZATION
                - transp / h * LATENT_HEAT_VAPORIZATION) * h / g['leaf_heat_capacity_j_k'])
    dt_wall = ((light_wall - wall_air + g['wall_outside_conductance_w_k'] * (f['outside_temperature_c'] - t_wall)
                + wall_flux / h * LATENT_HEAT_VAPORIZATION) * h / g['wall_heat_capacity_j_k'])
    dt_sub = ((-substrate_air - soil_evap / h * LATENT_HEAT_VAPORIZATION) * h / g['substrate_heat_capacity_j_k'])
    new_air, new_wall, new_leaf, new_sub = t_air + dt_air, t_wall + dt_wall, t_leaf + dt_leaf, t_sub + dt_sub
    excess = torch.clamp(vapor_new - _saturation_density(new_air) * v, min=0)
    vapor_new = vapor_new - excess
    fog = fog + excess
    new_air = new_air + excess * LATENT_HEAT_VAPORIZATION / air_capacity

    mol_air = f['atmospheric_pressure_pa'] * v / (8.314462618 * (t_air + 273.15))
    co2_injection = (e['co2_injection_g_h'] / 3600.0 * f['co2_doser'] / 44.01) * 1_000_000.0 / mol_air
    photosyn = (b['maximum_co2_assimilation_umol_m2_s'] * g['canopy_area_m2'] * light_response
                * s['co2_ppm'] / (s['co2_ppm'] + b['co2_half_saturation_ppm'])
                * torch.exp(-0.5 * ((t_leaf - b['photosynthesis_optimal_leaf_temperature_c'])
                                     / b['photosynthesis_temperature_width_c']) ** 2) * soil_response)
    respiration = b['respiration_umol_m2_s'] * g['canopy_area_m2']
    biological_co2 = (respiration - photosyn) / mol_air
    co2 = torch.clamp(s['co2_ppm'] + (q / v * (f['outside_co2_ppm'] - s['co2_ppm'])
                                        + co2_injection + biological_co2) * h, min=0)
    updated_soil = s['substrate_water_kg'] + f['irrigation_kg_s'] * h - transp - soil_evap
    capacity = b['substrate_water_capacity_kg']
    drainage = torch.clamp(updated_soil - capacity, min=0)
    updated_soil = torch.minimum(capacity, updated_soil)

    return {
        **s,
        'air_temperature_c': new_air,
        'wall_temperature_c': new_wall,
        'leaf_temperature_c': new_leaf,
        'substrate_temperature_c': new_sub,
        'air_vapor_kg': vapor_new,
        'co2_ppm': co2,
        'substrate_water_kg': updated_soil,
        'wall_condensate_kg': s['wall_condensate_kg'] + wall_flux,
        'leaf_condensate_kg': s['leaf_condensate_kg'] + leaf_flux,
        'cooler_condensate_kg': s['cooler_condensate_kg'] + coil_condensate,
        'dehumidifier_condensate_kg': s['dehumidifier_condensate_kg'] + dehumidifier,
        'drainage_kg': s['drainage_kg'] + drainage,
        'fog_condensate_kg': fog,
        'cumulative_net_external_water_kg': s['cumulative_net_external_water_kg'] + water_vent,
        'cumulative_added_water_kg': s['cumulative_added_water_kg'] + humidifier + f['irrigation_kg_s'] * h,
        'elapsed_s': s['elapsed_s'] + h,
        'cumulative_dli_mol_m2': s['cumulative_dli_mol_m2'] + ppfd * h * 1.0e-6,
    }


def simulate_batch(parameters: Sequence[GrowboxPhysics], states: Sequence[GrowboxState],
                   forcings: Sequence[GrowboxForcing], *, dt_s: float, ticks: int,
                   device: str = 'cpu', dtype: torch.dtype = torch.float64) -> BatchResult:
    """Batch heterogeneous scenarios on a requested device, fail-closed on errors.

    Different scenarios keep exactly their own scalar-reference Euler substep count
    and h; inactive elements are masked, avoiding a changed integration scheme.
    This MVP supports constant forcing for each trajectory across `ticks`.
    """
    if not (len(parameters) == len(states) == len(forcings)) or not states:
        raise ValueError('nonempty equal-length parameters, states and forcings required')
    if not isinstance(ticks, int) or ticks <= 0 or not math.isfinite(dt_s) or not 0 < dt_s <= 3600:
        raise ValueError('ticks > 0 and 0 < dt_s <= 3600 required')
    if dtype not in _FLOAT_TYPES:
        raise ValueError('use float64 (parity) or float32 (benchmark)')
    dev = torch.device(device)
    if dev.type not in ('cpu', 'cuda') or (dev.type == 'cuda' and (not torch.cuda.is_available()
             or (dev.index is not None and dev.index >= torch.cuda.device_count()))):
        raise RuntimeError(f'requested device {device} unavailable; no CPU fallback')
    # Reference substep schedules come from the same scalar integrator and validate
    # all initial state/forcing prerequisites. This is O(N) setup, not O(N*ticks).
    models = [GrowboxThermodynamics(p, s) for p, s in zip(parameters, states)]
    for m, f in zip(models, forcings):
        f.validate()
        if not -40 <= m.state.air_temperature_c <= 80:
            raise ValueError('air outside modeled temperature range')
    substeps = []
    for m, f in zip(models, forcings):
        g, e = m.parameters.geometry, m.parameters.equipment
        exchange = (m._exhaust(f) / 3600 / g.air_volume_m3 + e.infiltration_ach / 3600)
        c_air = m._reference_air_density_kg_m3 * g.air_volume_m3 * 1005 * g.air_heat_capacity_multiplier
        air_ua = 4 * (g.leaf_air_conductance_w_k + g.wall_air_conductance_w_k + g.substrate_air_conductance_w_k)
        characteristic = min(2, 0.05 * c_air / max(1e-12, air_ua), 0.05 / max(1e-12, exchange),
                             0.05 * g.wall_heat_capacity_j_k / max(1e-12, 4 * g.wall_air_conductance_w_k + g.wall_outside_conductance_w_k),
                             0.05 * g.leaf_heat_capacity_j_k / max(1e-12, 4 * g.leaf_air_conductance_w_k),
                             0.05 * g.substrate_heat_capacity_j_k / max(1e-12, 4 * g.substrate_air_conductance_w_k))
        n = max(1, math.ceil(dt_s / characteristic))
        if n > 30_000:
            raise ValueError('physics timestep requires excessive substeps')
        substeps.append(n)
    s = {name: _stack(states, name, dev, dtype) for name in _STATE_NAMES}
    g = _parameter_fields(parameters, 'geometry', dev, dtype)
    l = _parameter_fields(parameters, 'lighting', dev, dtype)
    b = _parameter_fields(parameters, 'biology', dev, dtype)
    e = _parameter_fields(parameters, 'equipment', dev, dtype)
    f = {field.name: (_optional(forcings, field.name, dev, dtype) if any(getattr(x, field.name) is None for x in forcings)
                      else _stack(forcings, field.name, dev, dtype)) for field in fields(GrowboxForcing)}
    # Fixed reference air density per scenario, as in the scalar oracle.
    ref_density = torch.tensor([m._reference_air_density_kg_m3 for m in models],device=dev,dtype=dtype)
    h = torch.tensor([dt_s / n for n in substeps], device=dev, dtype=dtype)
    counts = torch.tensor(substeps, device=dev, dtype=torch.int32)
    init_water = torch.tensor([m.initial_water_kg for m in models],device=dev,dtype=dtype)
    with torch.no_grad():
        for _ in range(ticks):
            for j in range(max(substeps)):
                newer = _substep(s, f, g, e, b, l, ref_density, h)
                active = j < counts
                s = {name: torch.where(active, newer[name], value) for name, value in s.items()}
        temperatures = torch.stack([s[k] for k in (
            'air_temperature_c', 'wall_temperature_c', 'leaf_temperature_c', 'substrate_temperature_c')])
        water_total = sum(s[k] for k in ('air_vapor_kg', 'substrate_water_kg', 'wall_condensate_kg',
                      'leaf_condensate_kg', 'drainage_kg', 'cooler_condensate_kg',
                      'dehumidifier_condensate_kg', 'fog_condensate_kg'))
        residual = water_total - s['cumulative_net_external_water_kg'] - s['cumulative_added_water_kg'] - init_water
        valid = torch.isfinite(torch.stack(tuple(s.values()))).all() & torch.isfinite(residual).all()
        valid &= ((temperatures >= -40) & (temperatures <= 80)).all()
        valid &= (s['air_vapor_kg'] >= 0).all() & (s['substrate_water_kg'] >= 0).all()
        if not bool(valid.item()):
            raise ValueError('tensor physics diverged or violated a physical bound')
        # Transfer only at the public boundary: no intermediate device->CPU sync.
        host = torch.stack([s[k] for k in _STATE_NAMES],dim=1).cpu().tolist()
        host_residual = residual.cpu().tolist()
    return BatchResult(states=tuple(GrowboxState(**dict(zip(_STATE_NAMES, row))) for row in host),
                       water_residuals_kg=tuple(host_residual), device=str(dev), dtype=str(dtype),
                       scenarios=len(states), ticks=ticks, substeps_per_tick=tuple(substeps))
