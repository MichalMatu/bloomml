"""Research sensor registry and provenance-aware physical forcing adapter.

Values from real sensors must retain provenance, sensor age and quality.
A synthetic, stale or unverified measurement must never silently become
calibration ground truth. Registries cover potential instruments, NOT claims
that a current BloomML board actually has all those sensors.
"""

from __future__ import annotations

import math
from dataclasses import dataclass, replace
from typing import Literal, Mapping

from .growbox_dynamics import GrowboxForcing

Source = Literal["measured", "calibrated_estimate", "synthetic"]


@dataclass(frozen=True)
class Channel:
    name: str
    unit: str
    low: float
    high: float
    category: str
    notes: str = ""


# No duplicate VPD/RH sensor channels: dewpoint, VPD, absolute humidity and
# DLI are DERIVED from measured channels, not invented extra instrumentation.
CHANNELS: tuple[Channel, ...] = (
    Channel("air_temperature_c", "degC", -30, 70, "chamber"),
    Channel("air_rh_pct", "%", 0, 100, "chamber"),
    Channel("air_co2_ppm", "ppm", 0, 10000, "chamber"),
    Channel("air_temperature_top_c", "degC", -30, 70, "spatial"),
    Channel("air_temperature_bottom_c", "degC", -30, 70, "spatial"),
    Channel("air_rh_top_pct", "%", 0, 100, "spatial"),
    Channel("air_rh_bottom_pct", "%", 0, 100, "spatial"),
    Channel("air_velocity_m_s", "m/s", 0, 25, "airflow"),
    Channel("exhaust_flow_m3_h", "m3/h", 0, 3000, "airflow"),
    Channel("exhaust_fan_rpm", "rpm", 0, 10000, "airflow"),
    Channel("differential_pressure_pa", "Pa", -5000, 5000, "airflow"),
    Channel("atmospheric_pressure_pa", "Pa", 50000, 110000, "outdoor"),
    Channel("outside_temperature_c", "degC", -40, 70, "outdoor"),
    Channel("outside_rh_pct", "%", 0, 100, "outdoor"),
    Channel("outside_co2_ppm", "ppm", 0, 10000, "outdoor"),
    Channel("outside_wind_speed_m_s", "m/s", 0, 60, "outdoor"),
    Channel("solar_irradiance_w_m2", "W/m2", 0, 1400, "light"),
    Channel("solar_ppfd_umol_m2_s", "umol/m2/s", 0, 3500, "light"),
    Channel("canopy_ppfd_umol_m2_s", "umol/m2/s", 0, 3500, "light"),
    Channel("canopy_ppfd_upper_umol_m2_s", "umol/m2/s", 0, 3500, "spatial"),
    Channel("canopy_ppfd_lower_umol_m2_s", "umol/m2/s", 0, 3500, "spatial"),
    Channel("canopy_illuminance_lux", "lx", 0, 250000, "light", "Lux cannot be converted to PPFD without a calibrated spectrum"),
    Channel("red_photon_flux_umol_m2_s", "umol/m2/s", 0, 3500, "spectrum"),
    Channel("blue_photon_flux_umol_m2_s", "umol/m2/s", 0, 3500, "spectrum"),
    Channel("far_red_photon_flux_umol_m2_s", "umol/m2/s", 0, 3500, "spectrum"),
    Channel("uva_w_m2", "W/m2", 0, 500, "spectrum"),
    Channel("uvb_w_m2", "W/m2", 0, 200, "spectrum"),
    Channel("lamp_electrical_power_w", "W", 0, 3000, "energy"),
    Channel("heater_electrical_power_w", "W", 0, 3000, "energy"),
    Channel("fan_electrical_power_w", "W", 0, 1000, "energy"),
    Channel("cooler_electrical_power_w", "W", 0, 3000, "energy"),
    Channel("humidifier_electrical_power_w", "W", 0, 1500, "energy"),
    Channel("dehumidifier_electrical_power_w", "W", 0, 2000, "energy"),
    Channel("lamp_surface_temperature_c", "degC", -20, 150, "surface"),
    Channel("wall_inner_temperature_c", "degC", -30, 90, "surface"),
    Channel("wall_outer_temperature_c", "degC", -30, 90, "surface"),
    Channel("leaf_temperature_c", "degC", -20, 70, "plant"),
    Channel("leaf_wetness_pct", "%", 0, 100, "plant"),
    Channel("leaf_area_m2", "m2", 0, 10, "plant"),
    Channel("substrate_temperature_c", "degC", -10, 60, "substrate"),
    Channel("substrate_moisture_vol_pct", "%", 0, 100, "substrate"),
    Channel("substrate_mass_kg", "kg", 0, 200, "substrate"),
    Channel("substrate_ec_ms_cm", "mS/cm", 0, 30, "substrate"),
    Channel("substrate_ph", "pH", 0, 14, "substrate"),
    Channel("substrate_drain_mass_kg", "kg", 0, 200, "substrate"),
    Channel("reservoir_temperature_c", "degC", 0, 60, "water"),
    Channel("reservoir_level_l", "L", 0, 500, "water"),
    Channel("irrigation_flow_l_h", "L/h", 0, 1000, "water"),
    Channel("humidifier_water_loss_g_h", "g/h", 0, 2000, "water"),
    Channel("condensate_rate_g_h", "g/h", 0, 2000, "water"),
    Channel("coil_surface_temperature_c", "degC", -40, 60, "surface"),
    Channel("co2_flow_g_h", "g/h", 0, 3000, "gas"),
    Channel("co2_cylinder_mass_kg", "kg", 0, 200, "gas"),
)
CHANNEL_BY_NAME = {channel.name: channel for channel in CHANNELS}


@dataclass(frozen=True)
class Measurement:
    name: str
    value: float
    sensor_id: str
    observed_at_ms: int
    source: Source
    valid: bool = True

    def __post_init__(self) -> None:
        if self.name not in CHANNEL_BY_NAME:
            raise ValueError(f"unregistered measurement {self.name}")
        spec = CHANNEL_BY_NAME[self.name]
        if (not self.sensor_id or not math.isfinite(self.value)
                or not spec.low <= self.value <= spec.high):
            raise ValueError(f"out of range / missing provenance: {self.name}")
        if type(self.observed_at_ms) is not int or self.observed_at_ms < 0:
            raise ValueError("invalid measurement timestamp")


# Only physical boundary/forcing channels can be ingested into the evolution
# model. Chamber and leaf measurements are for calibration/evaluation, not
# overwritten into simulated state during each predictive step.
BOUNDARY_FIELDS: dict[str, str] = {
    "outside_temperature_c": "outside_temperature_c",
    "outside_rh_pct": "outside_relative_humidity_pct",
    "outside_co2_ppm": "outside_co2_ppm",
    "atmospheric_pressure_pa": "atmospheric_pressure_pa",
    "exhaust_flow_m3_h": "measured_exhaust_m3_h",
    "air_velocity_m_s": "measured_air_speed_m_s",
    "differential_pressure_pa": "measured_pressure_difference_pa",
    "lamp_electrical_power_w": "lamp_measured_electrical_w",
    "canopy_ppfd_umol_m2_s": "canopy_measured_ppfd_umol_m2_s",
    "solar_irradiance_w_m2": "solar_irradiance_w_m2",
    "solar_ppfd_umol_m2_s": "solar_measured_ppfd_umol_m2_s",
}


def measured_forcing(
    readings: Mapping[str, Measurement], *,
    now_ms: int, max_age_ms: int = 30_000,
    nominal: GrowboxForcing | None = None,
) -> tuple[GrowboxForcing, dict[str, str]]:
    """Only verified, current measurements update a model boundary.

    Returns an independent status report so missing data cannot masquerade as
    current sensor truth. A caller may reject the nominal fallback in real
    calibration jobs.
    """
    if type(now_ms) is not int or now_ms < 0 or max_age_ms < 0:
        raise ValueError("invalid age/timestamp")
    forcing = nominal or GrowboxForcing()
    status: dict[str, str] = {}
    for name, attr in BOUNDARY_FIELDS.items():
        reading = readings.get(name)
        if reading is None:
            status[name] = "missing"
        elif reading.name != name:
            status[name] = "channel-mismatch"
        elif not reading.valid:
            status[name] = "invalid"
        elif reading.source != "measured":
            status[name] = "not-measured"
        elif reading.observed_at_ms > now_ms:
            status[name] = "future"
        elif now_ms - reading.observed_at_ms > max_age_ms:
            status[name] = "stale"
        else:
            status[name] = "measured"
            forcing = replace(forcing, **{attr: reading.value})
    forcing.validate()
    return forcing, status
