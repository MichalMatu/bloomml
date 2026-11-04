"""Source-mirror and chronological forecast tests; stdlib only, no download."""
from __future__ import annotations

import csv
from io import StringIO
from pathlib import Path
import tempfile
import unittest
from zipfile import ZipFile, ZIP_DEFLATED

from tools.ml.wur_external_benchmark import (
    SITES, MEASURED_TARGETS, HORIZONS_MIN, audit_archive, excel_tick,
)


def synthetic_archive(path: Path, *, n_days: int = 12, missing_house: str = "",
                      duplicate: bool = False, missing_column: bool = False,
                      nan_measurement: bool = False, extra_weather_missing: bool = False) -> None:
    with ZipFile(path, "w", compression=ZIP_DEFLATED) as z:
        for site in [*SITES, "Weather"]:
            if site == missing_house:
                continue
            keys = ["%time", "Tout", "Rhout", "PARout", "Iglob"] if site == "Weather" else [
                "%time", *MEASURED_TARGETS, "co2_dos", "co2_sp", "co2_vip", "VentLee"]
            if missing_column and site == SITES[0]:
                keys.remove("Tair")
            output = StringIO()
            wr = csv.DictWriter(output, fieldnames=keys)
            wr.writeheader()
            for day in range(n_days):
                for step in range(26):
                    tick = day * 288 + step
                    excel_day = 43815.0 + tick / 288
                    if duplicate and site == SITES[0] and day == n_days - 1 and step == 25:
                        excel_day = 43815 + (tick - 1) / 288
                    row = {"%time": f"{excel_day:.5f}"}
                    if site == "Weather":
                        row.update(Tout=9, Rhout=55, PARout=300, Iglob=170)
                        if extra_weather_missing and step == 0:
                            row["Tout"] = "NaN"
                    else:
                        row.update(Tair=20 + step / 10, Rhair=60 + step / 10,
                                   CO2air=400 + step, Tot_PAR=200 + step,
                                   co2_dos=0, co2_sp="NaN", co2_vip=400,
                                   VentLee=5)
                        if nan_measurement and step == 2:
                            row["CO2air"] = "NaN"
                    wr.writerow({key: row[key] for key in keys})
            z.writestr(f"{site}/Weather.csv" if site == "Weather" else
                       f"{site}/GreenhouseClimate.csv", output.getvalue())


class TestWurMirrorBenchmark(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.archive = Path(self.temp.name) / "wur-mirror.zip"

    def run_audit(self, **kwargs):
        synthetic_archive(self.archive, **kwargs)
        return audit_archive(self.archive, expected_sha256="")

    def test_6_houses_real_signal_boundaries_and_total_rows(self):
        r = self.run_audit()
        self.assertEqual(r["total_climate_rows"], 6 * 12 * 26)
        self.assertEqual(len(r["sites"]), 6)
        self.assertEqual(r["weather"]["rows"], 12 * 26)
        self.assertEqual(r["original_archive_byte_equivalence"], "NOT_VERIFIED")
        self.assertFalse(r["physical_growbox_calibration"])
        self.assertFalse(r["timezone_confirmed"])
        self.assertEqual(r["training_approval"], "EXPLORATORY_ONLY_ORIGINAL_VERIFICATION_PENDING")
        self.assertTrue(r["provenance"].startswith("measured_external"))
        self.assertIn("setpoints_not_proven_physical_airflow", r["control_column_semantics"])
        self.assertEqual(r["weather"]["source_columns"], 5)
        for row in r["sites"]:
            self.assertEqual(row["weather_matching_exact_timestamps"], 12 * 26)
            self.assertEqual(row["coverage"]["co2_ppm"], 1.0)
            self.assertEqual(row["source_columns"], 9)

    def test_persistence_5_15_60_minutes_per_day(self):
        r = self.run_audit()
        b = r["sites"][0]["baseline"]
        self.assertEqual(r["sites"][0]["holdout_rows"], 3 * 26)
        self.assertEqual(b["co2_ppm@5min"]["n"], 3 * 25)
        self.assertAlmostEqual(b["co2_ppm@5min"]["mae"], 1.0)
        self.assertEqual(b["co2_ppm@15min"]["n"], 3 * 23)
        self.assertAlmostEqual(b["co2_ppm@15min"]["mae"], 3.0)
        self.assertEqual(b["co2_ppm@60min"]["n"], 3 * 14)
        self.assertAlmostEqual(b["co2_ppm@60min"]["mae"], 12.0)
        self.assertEqual(b["temperature_c@60min"]["n"], 3 * 14)
        self.assertAlmostEqual(b["temperature_c@60min"]["mae"], 1.2)
        self.assertEqual(r["sites"][0]["unsupported_horizons_min"], [1])
        self.assertEqual(HORIZONS_MIN, (5, 15, 60))

    def test_nan_kept_missing_never_imputed(self):
        r = self.run_audit(nan_measurement=True)
        a = r["sites"][0]
        self.assertEqual(a["missing_values"]["CO2air"], 12)
        self.assertLess(a["coverage"]["co2_ppm"], 1)
        self.assertEqual(a["invalid_numeric_values"].get("CO2air", 0), 0)
        self.assertLess(a["baseline"]["co2_ppm@5min"]["n"], 3 * 25)

    def test_weather_na_does_not_turn_into_false_observation(self):
        r = self.run_audit(extra_weather_missing=True)
        self.assertEqual(r["weather"]["missing_values"]["Tout"], 12)
        self.assertTrue(r["weather"]["coverage"]["Tout"] < 1)

    def test_reject_duplicate_or_reversed_tick(self):
        with self.assertRaisesRegex(ValueError, "duplicate or reversed"):
            self.run_audit(duplicate=True)

    def test_reject_missing_or_conflicting_schema(self):
        with self.assertRaisesRegex(ValueError, "schema differs"):
            self.run_audit(missing_column=True)
        with self.assertRaisesRegex(ValueError, "source member missing"):
            self.run_audit(missing_house="Reference")

    def test_sha_mismatch_fail_closed(self):
        synthetic_archive(self.archive)
        with self.assertRaisesRegex(ValueError, "SHA256 mismatch"):
            audit_archive(self.archive, expected_sha256="a" * 64)

    def test_reject_too_short_time_series(self):
        with self.assertRaisesRegex(ValueError, "too few days"):
            self.run_audit(n_days=4)

    def test_excel_serial_is_not_silently_claimed_utc(self):
        self.assertEqual(excel_tick("43815.00000"), 43815 * 288)
        self.assertEqual(excel_tick("43815.00347"), 43815 * 288 + 1)
        with self.assertRaisesRegex(ValueError, "five-minute aligned"):
            excel_tick("43815.001")
        with self.assertRaisesRegex(ValueError, "out-of-domain"):
            excel_tick("Inf")
        with self.assertRaisesRegex(ValueError, "out-of-domain"):
            excel_tick("12345")

    def test_reject_bad_holdout_parameter(self):
        synthetic_archive(self.archive)
        with self.assertRaisesRegex(ValueError, "invalid chronological holdout"):
            audit_archive(self.archive, expected_sha256="", holdout_fraction=.99)


if __name__ == "__main__":
    unittest.main()
