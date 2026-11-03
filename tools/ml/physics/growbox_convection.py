"""Mixed-convection transfer coefficients for the zero-dimensional growbox twin.

Heat transfer from surface to bulk air is modeled by a forced flat-plate
Nusselt correlation, blended with a vertical free-convection engineering
correlation. Convection *conductances* remain calibrated coefficients: ratios
only scale the original reference speed/temperature difference. These
correlations approximate near-surface transport, NOT a CFD velocity field.
"""

from __future__ import annotations

import math

AIR_K_W_M_K = 0.0263
AIR_KINEMATIC_VISCOSITY_M2_S = 1.56e-5
AIR_PRANDTL = 0.71
REFERENCE_SPEED_M_S = 0.25
REFERENCE_DELTA_T_K = 2.0


def _h_w_m2_k(speed_m_s: float, delta_t_k: float, length_m: float) -> float:
    if not all(math.isfinite(value) for value in (speed_m_s, delta_t_k, length_m)):
        raise ValueError("non-finite convection inputs")
    if speed_m_s < 0 or length_m <= 0:
        raise ValueError("invalid convection speed or length")
    re = speed_m_s * length_m / AIR_KINEMATIC_VISCOSITY_M2_S
    # Laminar flat-plate average Nusselt for Re<5e5; turbulent transition
    # uses a standard empirical average-plate correlation. Both assume a
    # sufficiently unobstructed stream and still require local calibration.
    if re <= 500_000:
        nu_forced = 0.664 * math.sqrt(re) * AIR_PRANDTL ** (1 / 3)
    else:
        nu_forced = max(0.0, 0.037 * re ** 0.8 - 871.0) * AIR_PRANDTL ** (1 / 3)
    h_forced = nu_forced * AIR_K_W_M_K / length_m
    # Free-convection vertical-surface correlation (empirical): preserve
    # a heat path even with no circulation fan; deltas in kelvin.
    h_natural = 1.31 * (abs(delta_t_k) / length_m) ** (1 / 3)
    return (h_forced ** 3 + h_natural ** 3) ** (1 / 3)


def mixed_convection_factor(
    speed_m_s: float,
    surface_temperature_c: float,
    air_temperature_c: float,
    *,
    characteristic_length_m: float,
) -> float:
    """Dimensionless multiplier of calibrated UA, normalized at 0.25 m/s, 2 K.

    Bounds cap extrapolation when an instrument reports very high local speed;
    calibration must still be conducted for the actual enclosure topology.
    """
    reference = _h_w_m2_k(
        REFERENCE_SPEED_M_S, REFERENCE_DELTA_T_K, characteristic_length_m
    )
    current = _h_w_m2_k(
        speed_m_s, surface_temperature_c - air_temperature_c,
        characteristic_length_m,
    )
    return min(4.0, max(0.0, current / reference))


def effective_canopy_air_speed_m_s(
    *,
    measured_air_speed_m_s: float | None,
    exhaust_command: float,
    circulation_command: float,
    background_air_speed_m_s: float,
    exhaust_induced_air_speed_m_s: float,
    circulation_air_speed_m_s: float,
) -> tuple[float, str]:
    """Measured local speed wins; otherwise return labeled empirical estimate.

    A 0-D model cannot resolve stratification, jets, dead zones or direction;
    measured sensor readings should be tied to the local canopy position.
    """
    if measured_air_speed_m_s is not None:
        if not math.isfinite(measured_air_speed_m_s) or measured_air_speed_m_s < 0:
            raise ValueError("invalid measured air speed")
        return measured_air_speed_m_s, "measured"
    for v in (exhaust_command, circulation_command, background_air_speed_m_s,
              exhaust_induced_air_speed_m_s, circulation_air_speed_m_s):
        if not math.isfinite(v) or v < 0:
            raise ValueError("invalid estimated airflow input")
    # Root-sum-square avoids assuming perfectly aligned ducts/air streams.
    speed = math.sqrt(
        background_air_speed_m_s ** 2
        + (exhaust_command * exhaust_induced_air_speed_m_s) ** 2
        + (circulation_command * circulation_air_speed_m_s) ** 2
    )
    return speed, "estimated"
