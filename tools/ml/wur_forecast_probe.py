"""Exploratory leave-one-greenhouse-out *and* chronological forecasting probe.

A single-parameter trend model, fitted ONLY on earlier days of five *other*
greenhouses. Test on last 20% of days in one entirely unseen house. This is a
source-mirror diagnostic, not validation of the physics solver or production ML.
"""
from __future__ import annotations

import argparse
from collections import defaultdict
import json
from pathlib import Path
from zipfile import ZipFile

from .wur_external_benchmark import (
    MIRROR_SHA256, MEASURED_TARGETS, SITES, HORIZONS_MIN, audit_member,
    baseline, tick_date, audit_archive,
)


def _fit_sums(values: dict[int, dict], target: str, horizon_ticks: int,
              cutoff: str) -> tuple[float, float, int]:
    cross = sq = 0.0
    count = 0
    for tick, current in values.items():
        if tick_date(tick) >= cutoff or tick_date(tick + horizon_ticks) >= cutoff:
            continue
        prev, future = values.get(tick - 1), values.get(tick + horizon_ticks)
        if prev is None or future is None:
            continue
        a, b, c = prev[target], current[target], future[target]
        if a is None or b is None or c is None:
            continue
        derivative = b - a
        future_delta = c - b
        cross += derivative * future_delta
        sq += derivative * derivative
        count += 1
    return cross, sq, count


def _test(values: dict[int, dict], target: str, h_ticks: int,
          cutoff: str, alpha: float, lo: float, hi: float) -> dict:
    persistence, trend = [], []
    for tick, current in values.items():
        # Hold out forecast origins AND their measured 5-min history.
        if tick_date(tick) < cutoff or tick_date(tick - 1) < cutoff:
            continue
        prev, future = values.get(tick - 1), values.get(tick + h_ticks)
        if prev is None or future is None:
            continue
        a, b, c = prev[target], current[target], future[target]
        if a is None or b is None or c is None:
            continue
        persistence.append(b - c)
        forecast = min(hi, max(lo, b + alpha * (b - a)))
        trend.append(forecast - c)
    p = baseline(persistence)
    m = baseline(trend)
    assert p["n"] == m["n"]
    rel = (100.0 * (p["mae"] - m["mae"]) / p["mae"]
           if p["n"] > 0 and p["mae"] > 0 else None)
    return {"persistence": p, "trained_trend": m,
            "improvement_mae_percent": rel, "comparable_test_pairs": p["n"]}


def probe_archive(zipfile: Path, *, expected_sha256: str = MIRROR_SHA256) -> dict:
    # First validate entire ZIP, time, columns, and all member ranges.
    evidence = audit_archive(zipfile, expected_sha256=expected_sha256)
    if evidence["training_approval"] != "EXPLORATORY_ONLY_ORIGINAL_VERIFICATION_PENDING":
        raise ValueError("unrecognized source provenance")
    stats = {row["site"]: row for row in evidence["sites"]}
    records: dict[str, dict[int, dict]] = {}
    with ZipFile(zipfile) as z:
        for site in SITES:
            _, values = audit_member(z, f"{site}/GreenhouseClimate.csv",
                                     MEASURED_TARGETS, holdout_fraction=.2)
            records[site] = values
    fits: dict[tuple[str, int, str], tuple[float, float, int]] = {}
    for site in SITES:
        cutoff = stats[site]["holdout_cutoff_naive_day"]
        for target in MEASURED_TARGETS:
            for horizon in HORIZONS_MIN:
                fits[(site, horizon, target)] = _fit_sums(
                    records[site], target, horizon // 5, cutoff)
    reports = []
    for test_site in SITES:
        row = {"holdout_site": test_site,
               "holdout_naive_day": stats[test_site]["holdout_cutoff_naive_day"],
               "fit_other_sites": [s for s in SITES if s != test_site],
               "targets": {}}
        for target, (label, lo, hi) in MEASURED_TARGETS.items():
            for horizon in HORIZONS_MIN:
                numerator, denominator, ntrain = 0.0, 0.0, 0
                for train_site in SITES:
                    if train_site == test_site:
                        continue
                    a, b, c = fits[(train_site, horizon, target)]
                    numerator += a
                    denominator += b
                    ntrain += c
                if ntrain < 100 or denominator <= 0:
                    alpha = 0.0
                else:
                    alpha = max(-4.0, min(4.0, numerator / denominator))
                score = _test(records[test_site], target, horizon // 5,
                              row["holdout_naive_day"], alpha, lo, hi)
                score["trained_coefficient"] = alpha
                score["train_pair_count_five_other_sites"] = ntrain
                score["formula"] = "bounded(y(t)+a*(y(t)-y(t-5min)))"
                row["targets"][f"{label}@{horizon}min"] = score
        reports.append(row)
    return {
        "schema": "bloomml-wur-leave-one-house-out-chronological-trend-v1",
        "data_source": evidence["mirror_ref"],
        "data_mirror_sha256": evidence["mirror_sha256"],
        "original_byte_equivalence": evidence["original_archive_byte_equivalence"],
        "real_world_growbox_validation": False,
        "training_approval": "DIAGNOSTIC_TRANSFER_ONLY_NOT_PRODUCTION",
        "hyperparameter_selection": "fixed_formula_alpha_OLS_unbounded_then_clamped_minus4_plus4",
        "split": "train_early_80_percent_days_of_other_five_houses_only;evaluate_late_20_percent_days_of_sixth",
        "predictor_features": ["observed_same_target_now", "observed_same_target_5min_ago"],
        "forbidden_features": ["future_weather", "future_setpoint", "future_measurements",
                               "target_house_train_labels", "derived_simulated_targets"],
        "comparison": "persistence_and_trend_on_exactly_same_test_rows",
        "reports": reports,
    }


def main() -> int:
    p = argparse.ArgumentParser(description="Independent cross-house + chronological WUR forecast probe")
    p.add_argument("--zip", type=Path, required=True)
    p.add_argument("--expected-sha256", default=MIRROR_SHA256)
    p.add_argument("--output", type=Path)
    args = p.parse_args()
    try:
        result = probe_archive(args.zip, expected_sha256=args.expected_sha256)
    except (OSError, ValueError) as exc:
        print(json.dumps({"status": "REJECTED", "reason": str(exc)}, ensure_ascii=False))
        return 2
    payload = json.dumps(result, indent=2, sort_keys=True, ensure_ascii=False) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(payload, encoding="utf-8")
    else:
        print(payload)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
