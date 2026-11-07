"""Conservative, temperature-coupled well-mixed fog/vapor phase balance.

The algebraic phase state is adjusted at a substep boundary. Condensation is
instantaneous in a zero-dimensional cell; fog re-evaporation is limited by a
specified time constant. This does not model aerosol size or cloud kinetics.
"""
from __future__ import annotations

import math

from .growbox_psychrometrics import (
    LATENT_HEAT_VAPORIZATION, saturation_vapor_density_kg_m3,
)

FOG_EVAPORATION_TIME_S = 60.0  # uncalibrated prior, independent of substep length


def _sat_density_slope(t_c: float) -> float:
    sat = saturation_vapor_density_kg_m3(t_c)
    return sat * (17.625 * 243.04 / (t_c + 243.04) ** 2 - 1.0 / (t_c + 273.15))


def phase_equilibrium(*, vapor_kg: float, fog_kg: float, air_temperature_c: float,
                      volume_m3: float, air_capacity_j_k: float, dt_s: float,
                      iterations: int = 8) -> tuple[float, float, float]:
    """Return (vapor, fog, temperature) conserving total water and latent energy.

    Solves x + V*rho_sat(T + L*x/C) - m_v = 0, where x is condensate
    gain. The allowed negative x (fog evaporation) is time-rate limited.
    Clamp never manufactures liquid water. No hidden temperature corrections.
    """
    if (not all(math.isfinite(v) for v in (vapor_kg, fog_kg, air_temperature_c,
                                           volume_m3, air_capacity_j_k, dt_s))
            or min(vapor_kg, fog_kg) < 0 or min(volume_m3, air_capacity_j_k, dt_s) <= 0):
        raise ValueError("invalid phase equilibrium inputs")
    evaporation_limit = fog_kg * -math.expm1(-dt_s / FOG_EVAPORATION_TIME_S)
    temperature_to_mass = air_capacity_j_k / LATENT_HEAT_VAPORIZATION
    low = max(-evaporation_limit, (-39.9999 - air_temperature_c) * temperature_to_mass)
    high = min(vapor_kg, (79.9999 - air_temperature_c) * temperature_to_mass)
    if low > high:
        raise ValueError('phase equilibrium is outside the modeled temperature domain')
    x = min(high, max(low, vapor_kg - saturation_vapor_density_kg_m3(air_temperature_c) * volume_m3))
    for _ in range(iterations):
        t = air_temperature_c + x * LATENT_HEAT_VAPORIZATION / air_capacity_j_k
        f = x + saturation_vapor_density_kg_m3(t) * volume_m3 - vapor_kg
        df = 1.0 + _sat_density_slope(t) * volume_m3 * LATENT_HEAT_VAPORIZATION / air_capacity_j_k
        x = min(high, max(low, x - f / df))
    t_final = air_temperature_c + x * LATENT_HEAT_VAPORIZATION / air_capacity_j_k
    if vapor_kg - x > (1.0 + 1e-8) * saturation_vapor_density_kg_m3(t_final) * volume_m3:
        raise ValueError('saturated fog cannot be equilibrated within modeled temperature range')
    return vapor_kg - x, fog_kg + x, t_final
