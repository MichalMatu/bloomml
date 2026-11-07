"""SI psychrometrics for the research growbox energy-and-mass-balance model.

T in Celsius, P in Pa, water in kg, V in m^3. Saturation is over liquid
water (ice/frost requires a separate extension for sub-freezing surfaces).
"""

from __future__ import annotations

import math

R_DRY_AIR = 287.05  # J/(kg K)
R_WATER_VAPOR = 461.52  # J/(kg K)
LATENT_HEAT_VAPORIZATION = 2_450_000.0  # J/kg, nominal near 20 C


def saturation_pressure_pa(temperature_c: float) -> float:
    """Magnus saturation vapor pressure over liquid water, validated range."""
    if not -40.0 <= temperature_c <= 80.0 or not math.isfinite(temperature_c):
        raise ValueError("temperature outside liquid-water Magnus range [-40, 80] C")
    return 610.94 * math.exp(17.625 * temperature_c / (temperature_c + 243.04))


def saturation_vapor_density_kg_m3(temperature_c: float) -> float:
    return saturation_pressure_pa(temperature_c) / (R_WATER_VAPOR * (273.15 + temperature_c))


def vapor_density_kg_m3(temperature_c: float, relative_humidity_pct: float) -> float:
    if not 0.0 <= relative_humidity_pct <= 100.0 or not math.isfinite(relative_humidity_pct):
        raise ValueError("relative humidity outside 0-100 percent")
    return saturation_vapor_density_kg_m3(temperature_c) * relative_humidity_pct / 100.0


def vapor_pressure_pa(temperature_c: float, vapor_density: float) -> float:
    if vapor_density < 0.0 or not math.isfinite(vapor_density):
        raise ValueError("invalid vapor density")
    return vapor_density * R_WATER_VAPOR * (temperature_c + 273.15)


def relative_humidity_pct(temperature_c: float, vapor_density: float) -> float:
    return 100.0 * vapor_density / saturation_vapor_density_kg_m3(temperature_c)


def dew_point_c(vapor_pressure: float) -> float | None:
    """Dewpoint over liquid water. None for exactly dry air."""
    if vapor_pressure < 0.0 or not math.isfinite(vapor_pressure):
        raise ValueError("invalid vapor pressure")
    if vapor_pressure == 0.0:
        return None
    gamma = math.log(vapor_pressure / 610.94)
    return 243.04 * gamma / (17.625 - gamma)


def air_vpd_kpa(temperature_c: float, vapor_pressure: float) -> float:
    return (saturation_pressure_pa(temperature_c) - vapor_pressure) / 1000.0


def leaf_vpd_kpa(leaf_temperature_c: float, vapor_pressure: float) -> float:
    """May be negative when the leaf is below the local dewpoint."""
    return (saturation_pressure_pa(leaf_temperature_c) - vapor_pressure) / 1000.0


def moist_air_density_kg_m3(temperature_c: float, pressure_pa: float, vapor_pressure: float) -> float:
    if not 50_000 <= pressure_pa <= 110_000 or not math.isfinite(pressure_pa):
        raise ValueError("pressure outside supported atmosphere range")
    if vapor_pressure < 0 or vapor_pressure >= pressure_pa:
        raise ValueError("invalid vapor partial pressure")
    absolute_temperature = temperature_c + 273.15
    return (pressure_pa - vapor_pressure) / (R_DRY_AIR * absolute_temperature) + vapor_pressure / (R_WATER_VAPOR * absolute_temperature)
