"""External provenance, leakage and missing-data gates; stdlib unittest only."""
from __future__ import annotations
import csv
from datetime import datetime, timedelta
import hashlib
from pathlib import Path
import tempfile
import unittest

from tools.ml.external_greenhouse_benchmark import evaluate_file, SITES


def make_csv(path: Path, *, site_id="alava", days=10, missing=False, malformed=False,
             bad_clock=False, duplicate=False, outlier=False):
    spec = SITES[site_id]
    cols = ["dateTime", *spec.columns.values(), *spec.actuator_columns]
    rows = []
    start = datetime(2025, 1, 1)
    for day in range(days):
        for quarter in range(10):
            t = start + timedelta(days=day, minutes=15 * quarter)
            if bad_clock:
                stamp = t.isoformat() + "+00:00"
            else:
                stamp = t.isoformat(sep=" ")
            row = {column: "" for column in cols}
            row["dateTime"] = stamp
            row[spec.columns["air_temperature_c"]] = str(20 + quarter)
            row[spec.columns["relative_humidity_pct"]] = str(60 - quarter)
            if missing and day >= days - 2:
                row[spec.columns["relative_humidity_pct"]] = ""
            if outlier and day == days - 1:
                row[spec.columns["relative_humidity_pct"]] = "102"
            rows.append(row)
    if duplicate:
        rows[-1]["dateTime"] = rows[-2]["dateTime"]
    if malformed:
        cols.remove(spec.columns["air_temperature_c"])
    with path.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.DictWriter(fh, fieldnames=cols, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)


class TestExternalGreenhouseBenchmark(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.path = Path(self.tmp.name) / "file.csv"

    def test_holdout_and_leakage_safe_persistence(self):
        make_csv(self.path)
        r = evaluate_file(self.path, "alava")
        self.assertEqual(r["rows"], 100)
        self.assertEqual(r["holdout_cutoff_local_naive_day"], "2025-01-09")
        self.assertEqual(r["holdout_source_rows"], 20)
        self.assertEqual(r["baseline"]["air_temperature_c@15min"]["n"], 18)
        self.assertAlmostEqual(r["baseline"]["air_temperature_c@15min"]["mae"], 1)
        self.assertEqual(r["baseline"]["air_temperature_c@60min"]["n"], 12)
        self.assertAlmostEqual(r["baseline"]["air_temperature_c@60min"]["mae"], 4)
        self.assertAlmostEqual(r["baseline"]["relative_humidity_pct@60min"]["mae"], 4)
        self.assertEqual(r["unsupported_horizons_min"], [1,5])
        self.assertFalse(r["timezone_confirmed"])
        self.assertEqual(r["clock_basis"], "source_naive_timezone_unverified")
        self.assertFalse(r["physical_model_calibrated"])
        self.assertIn("NOT_APPROVED", r["training_approval"])

    def test_missing_is_never_forward_filled(self):
        make_csv(self.path, missing=True)
        r = evaluate_file(self.path, "alava")
        self.assertEqual(r["baseline"]["relative_humidity_pct@15min"]["n"], 0)
        self.assertEqual(r["baseline"]["relative_humidity_pct@15min"]["status"], "NO_COMPLETE_PAIRS")
        self.assertGreater(r["baseline"]["air_temperature_c@15min"]["n"], 0)

    def test_nonfinite_and_outlier_values_are_not_truth(self):
        make_csv(self.path, outlier=True)
        r = evaluate_file(self.path, "alava")
        self.assertEqual(r["invalid_numeric_counts"]["relative_humidity_pct"], 10)
        self.assertLess(r["completeness_fraction"]["relative_humidity_pct"], 1)

    def test_reject_duplicate_and_ambiguous_clock_mapping(self):
        make_csv(self.path, duplicate=True)
        with self.assertRaisesRegex(ValueError, "duplicate or nonmonotonic"):
            evaluate_file(self.path, "alava")
        make_csv(self.path, bad_clock=True)
        with self.assertRaisesRegex(ValueError, "unexpected time zone"):
            evaluate_file(self.path, "alava")

    def test_reject_wrong_schema_and_hash(self):
        make_csv(self.path, malformed=True)
        with self.assertRaisesRegex(ValueError, "schema missing"):
            evaluate_file(self.path, "alava")
        make_csv(self.path)
        with self.assertRaisesRegex(ValueError, "SHA256 mismatch"):
            evaluate_file(self.path, "alava", expected_sha256="f"*64)
        with self.assertRaisesRegex(ValueError, "unknown site"):
            evaluate_file(self.path, "unknown")

    def test_supported_sites_keep_external_domain_and_do_not_claim_actuation(self):
        for site in ("alava", "navarra_comp0", "cartagena_comp0"):
            make_csv(self.path, site_id=site)
            r=evaluate_file(self.path, site)
            self.assertIn("external", r["provenance"])
            self.assertEqual(r["actuator_truth"],
                "control_status_or_setpoint_not_physical_airflow_confirmation")
            self.assertEqual(r["source_url"], "https://zenodo.org/records/21037154")
            self.assertEqual(r["source_sha256"],hashlib.sha256(self.path.read_bytes()).hexdigest())

    def test_reject_small_or_unauditable_dataset(self):
        make_csv(self.path, days=2)
        with self.assertRaisesRegex(ValueError, "distinct days"):
            evaluate_file(self.path, "alava")
        make_csv(self.path)
        with self.assertRaisesRegex(ValueError, "holdout_fraction"):
            evaluate_file(self.path, "alava", holdout_fraction=0.9)


if __name__ == "__main__":
    unittest.main()
