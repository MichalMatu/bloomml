"""Leakage-proof diagnostic model probes on small synthetic WUR ZIPs."""
from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from tests.test_wur_external_benchmark import synthetic_archive
from tools.ml.wur_external_benchmark import excel_tick, SITES
from tools.ml.wur_forecast_probe import _fit_sums, _test, probe_archive


class TestWurForecastProbe(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name) / "test.zip"

    def test_train_on_five_houses_test_on_unseen_sixth(self):
        synthetic_archive(self.path)
        r = probe_archive(self.path, expected_sha256="")
        self.assertEqual(r["schema"], "bloomml-wur-leave-one-house-out-chronological-trend-v1")
        self.assertFalse(r["real_world_growbox_validation"])
        self.assertEqual(r["original_byte_equivalence"], "NOT_VERIFIED")
        self.assertEqual(len(r["reports"]), 6)
        for report in r["reports"]:
            self.assertNotIn(report["holdout_site"], report["fit_other_sites"])
            self.assertEqual(len(report["fit_other_sites"]), 5)
            self.assertEqual(len(report["targets"]), 12)
            for target in report["targets"].values():
                self.assertEqual(target["trained_trend"]["n"], target["persistence"]["n"])
                self.assertGreater(target["train_pair_count_five_other_sites"], 100)
                self.assertGreater(target["comparable_test_pairs"], 0)
                self.assertTrue(-4 <= target["trained_coefficient"] <= 4)
                self.assertIsNotNone(target["improvement_mae_percent"])
        self.assertNotIn("future_weather", r["predictor_features"])
        self.assertIn("future_weather", r["forbidden_features"])

    def test_fit_does_not_consume_holdout_target_or_future(self):
        tick = excel_tick("43815.00000")
        values = {tick + 0: {"CO2air": 400.},
                  tick + 1: {"CO2air": 401.},
                  tick + 2: {"CO2air": 402.},
                  tick + 288: {"CO2air": 500.},
                  tick + 289: {"CO2air": 600.},
                  tick + 290: {"CO2air": 1_000_000.}}
        cutoff = "2019-12-17"
        train = _fit_sums(values, "CO2air", 1, cutoff)
        self.assertEqual(train, (1., 1., 1))
        values[tick + 289]["CO2air"] = -3_000_000.
        self.assertEqual(_fit_sums(values, "CO2air", 1, cutoff), train)

    def test_evaluation_rejects_train_day_histories_and_uses_identical_pairs(self):
        tick = excel_tick("43815.00000")
        data = {tick: {"Tair": 10.},
                tick + 288: {"Tair": 11.},
                tick + 289: {"Tair": 12.},
                tick + 290: {"Tair": 13.}}
        report = _test(data, "Tair", 1, "2019-12-17", alpha=1, lo=-40, hi=80)
        self.assertEqual(report["comparable_test_pairs"], 1)
        self.assertAlmostEqual(report["persistence"]["mae"], 1)
        self.assertAlmostEqual(report["trained_trend"]["mae"], 0)

    def test_pinned_sha_is_mandatory_by_default(self):
        synthetic_archive(self.path)
        with self.assertRaisesRegex(ValueError, "SHA256 mismatch"):
            probe_archive(self.path)

    def test_original_source_not_qualified_as_same_bytes(self):
        synthetic_archive(self.path)
        r = probe_archive(self.path, expected_sha256="")
        self.assertIn("ORIGINAL_VERIFICATION_PENDING", r["training_approval"])
        self.assertEqual(r["original_byte_equivalence"], "NOT_VERIFIED")


if __name__ == "__main__":
    unittest.main()
