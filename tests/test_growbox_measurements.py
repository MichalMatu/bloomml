from dataclasses import replace

import pytest

from tools.ml.physics.growbox_measurements import (
    Measurement, measured_forcing, CHANNELS, CHANNEL_BY_NAME, BOUNDARY_FIELDS,
)
from tools.ml.physics.growbox_dynamics import GrowboxForcing


def test_registry_is_unique_and_sensor_units_are_preserved():
    assert len(CHANNEL_BY_NAME) == len(CHANNELS)
    assert len(CHANNELS) >= 45
    assert CHANNEL_BY_NAME["canopy_ppfd_umol_m2_s"].unit == "umol/m2/s"
    assert CHANNEL_BY_NAME["canopy_illuminance_lux"].unit == "lx"


def test_only_fresh_measured_boundary_updates_physics():
    inputs = {
        "lamp_electrical_power_w": Measurement("lamp_electrical_power_w", 141, "shelly-1", 9500, "measured"),
        "canopy_ppfd_umol_m2_s": Measurement("canopy_ppfd_umol_m2_s", 610, "par-1", 9500, "measured"),
        "outside_temperature_c": Measurement("outside_temperature_c", 8, "bme-1", 0, "measured"),
        "outside_rh_pct": Measurement("outside_rh_pct", 55, "sim-1", 9500, "synthetic"),
        "air_temperature_c": Measurement("air_temperature_c", 26, "scd41-1", 9500, "measured"),
    }
    f, status = measured_forcing(inputs, now_ms=10000, max_age_ms=1000)
    assert f.lamp_measured_electrical_w == 141
    assert f.canopy_measured_ppfd_umol_m2_s == 610
    assert f.outside_temperature_c == 20
    assert f.outside_relative_humidity_pct == 50
    assert status["outside_temperature_c"] == "stale"
    assert status["outside_rh_pct"] == "not-measured"
    assert "air_temperature_c" not in BOUNDARY_FIELDS


def test_faulty_timestamp_and_bad_unit_range_rejected():
    with pytest.raises(ValueError):
        Measurement("air_rh_pct", 120, "s", 1, "measured")
    with pytest.raises(ValueError):
        Measurement("air_temperature_c", 20, "", 1, "measured")
    f, status = measured_forcing({"lamp_electrical_power_w": Measurement(
        "lamp_electrical_power_w", 10, "power", 20, "measured")}, now_ms=10)
    assert status["lamp_electrical_power_w"] == "future"
    assert f.lamp_measured_electrical_w is None
