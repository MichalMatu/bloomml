"""Reproducible, read-only external greenhouse CSV audit and persistence baselines.

No remote I/O, credentials, Torch, firmware or synthetic interpolation.
The NUTRICONTROL clean CSVs contain naive dateTime values: their timezone
is not proven. Keep the original wall-clock string; do NOT label it UTC.
"""
from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
from datetime import datetime, timedelta
import hashlib
import json
import math
from pathlib import Path
from typing import NamedTuple

SOURCE_ID = "NUTRICONTROL_V1_2026"
SOURCE_URL = "https://zenodo.org/records/21037154"
DATA_LICENSE = "CC-BY-4.0"
CLOCK_BASIS = "source_naive_timezone_unverified"
HORIZONS_MINUTES = (15, 60)  # 1m/5m impossible in a 15m-resolution file
INVALID_RANGE = {"air_temperature_c": (-40.0, 80.0), "relative_humidity_pct": (0.0, 100.0)}

@dataclass(frozen=True)
class Site:
    name: str
    source_path: str
    columns: dict[str, str]
    actuator_columns: tuple[str, ...] = ()

SITES = {
    "alava": Site(
        "alava", "Alava_data/clean_data/Alava_15min.csv",
        {"air_temperature_c": "DB_TEMP_INT_Alava",
         "relative_humidity_pct": "REL_HUM_INT_Alava",
         "outside_temperature_c": "DB_TEMP_EXT_Alava",
         "outside_relative_humidity_pct": "REL_HUM_EXT_Alava",
         "solar_radiation_w_m2": "RADIATION_MEAN_Alava"},
        ("ACT_FORCED_VENT_STAGE_Alava", "WINDOW_0_Alava", "WINDOW_1_Alava")),
    "cartagena_comp0": Site(
        "cartagena_comp0", "Cartagena_data/clean_data/Cartagena_comp0_15min.csv",
        {"air_temperature_c": "DB_TEMP_INT_Cartagena_comp0",
         "relative_humidity_pct": "REL_HUM_INT_Cartagena_comp0"},
        ("DEST_Cartagena_comp0", "WINDOW_0_Cartagena_comp0",
         "WINDOW_1_Cartagena_comp0", "WINDOW_2_Cartagena_comp0")),
    "navarra_comp0": Site(
        "navarra_comp0", "Navarra_data/clean_data/Navarra_comp0_15min.csv",
        {"air_temperature_c": "DB_TEMP_INT_Navarra_comp0",
         "relative_humidity_pct": "REL_HUM_INT_Navarra_comp0"},
        ("DEST_Navarra_comp0", "HEATERS_Navarra_comp0",
         "WINDOW_0_Navarra_comp0")),
}

class Sample(NamedTuple):
    timestamp: datetime  # naive and explicitly unverified
    air_temperature_c: float | None
    relative_humidity_pct: float | None

def _number(raw: str | None, key: str) -> float | None:
    if raw is None or raw.strip().lower() in {"", "null", "nan", "none"}:
        return None
    try:
        value = float(raw)
    except ValueError as exc:
        raise ValueError(f"invalid numeric value in {key}") from exc
    if not math.isfinite(value):
        raise ValueError(f"nonfinite numeric value in {key}")
    if key in INVALID_RANGE:
        lo, hi = INVALID_RANGE[key]
        if not lo <= value <= hi:
            raise ValueError(f"{key} exceeds physical acceptance bounds")
    return value

def _time(raw: str) -> datetime:
    try:
        t = datetime.fromisoformat(raw.strip())
    except ValueError as exc:
        raise ValueError("invalid source timestamp") from exc
    if t.tzinfo is not None:
        raise ValueError("unexpected time zone offset: supply explicit verified clock contract")
    return t

def _metrics(errors: list[float]) -> dict:
    if not errors:
        return {"status": "NO_COMPLETE_PAIRS", "n": 0}
    return {"status": "BASELINE_ONLY", "n": len(errors),
            "mae": sum(abs(e) for e in errors) / len(errors),
            "rmse": math.sqrt(sum(e * e for e in errors) / len(errors)),
            "bias_pred_minus_observed": sum(errors) / len(errors)}

def evaluate_file(source: Path, site_id: str, *, expected_sha256: str | None = None,
                  holdout_fraction: float = .2) -> dict:
    """Audit one processed real greenhouse file without storing or imputing rows.

    Persistence is prediction y(t+h)=y(t), evaluated on chronologically held-out
    source times only. It is NOT a fitted or calibrated physical model.
    """
    if site_id not in SITES:
        raise ValueError(f"unknown site: {site_id}")
    if not .05 <= holdout_fraction <= .5:
        raise ValueError("holdout_fraction must be between 5 and 50 percent")
    if not source.is_file():
        raise ValueError("source CSV missing")
    sha = hashlib.sha256()
    with source.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            sha.update(chunk)
    if expected_sha256 is not None and sha.hexdigest() != expected_sha256:
        raise ValueError("external file SHA256 mismatch")
    site = SITES[site_id]
    observed: list[Sample] = []
    extra_counts = {key: 0 for key in site.columns if key not in INVALID_RANGE}
    actuator_counts = {key: 0 for key in site.actuator_columns}
    invalid_counts = {key: 0 for key in INVALID_RANGE}
    gap_counts: dict[str, int] = {}
    previous: datetime | None = None
    rows = 0
    with source.open("r", encoding="utf-8-sig", newline="") as f:
        reader = csv.DictReader(f)
        fieldnames = reader.fieldnames or []
        if len(fieldnames) != len(set(fieldnames)):
            raise ValueError("duplicate CSV column names")
        required = {"dateTime", site.columns["air_temperature_c"],
                    site.columns["relative_humidity_pct"]}
        if not required.issubset(set(fieldnames)):
            raise ValueError(f"source schema missing required columns: {sorted(required - set(fieldnames))}")
        for row in reader:
            rows += 1
            t = _time(row["dateTime"])
            if previous is not None:
                delta = int((t - previous).total_seconds())
                if delta <= 0:
                    raise ValueError("duplicate or nonmonotonic source timestamp")
                gap_counts[str(delta)] = gap_counts.get(str(delta), 0) + 1
            previous = t
            values = {}
            for key in INVALID_RANGE:
                try:
                    values[key] = _number(row[site.columns[key]], key)
                except ValueError:
                    values[key] = None
                    invalid_counts[key] += 1
            for key, column in site.columns.items():
                if key not in INVALID_RANGE and column in fieldnames:
                    try:
                        if _number(row.get(column), key) is not None:
                            extra_counts[key] += 1
                    except ValueError:
                        pass
            for column in site.actuator_columns:
                # Metadata counts only; never assume a command confirms flow/power.
                if column in fieldnames and row.get(column) not in (None, "", "nan"):
                    actuator_counts[column] += 1
            observed.append(Sample(t, values["air_temperature_c"],
                                    values["relative_humidity_pct"]))
    if rows < 20:
        raise ValueError("too few rows for chronological holdout")
    unique_days = sorted(set(s.timestamp.date() for s in observed))
    if len(unique_days) < 4:
        raise ValueError("too few distinct days for day-blocked holdout")
    n_holdout = max(1, math.ceil(len(unique_days) * holdout_fraction))
    cutoff_date = unique_days[-n_holdout]
    # Exact matching only, no implicit resampling/forward fill.
    index = {s.timestamp: s for s in observed}
    targets = ("air_temperature_c", "relative_humidity_pct")
    completeness = {key: sum(getattr(s, key) is not None for s in observed) / rows for key in targets}
    baseline = {}
    test_sources = [s for s in observed if s.timestamp.date() >= cutoff_date]
    for horizon in HORIZONS_MINUTES:
        for key in targets:
            errors = []
            for s in test_sources:
                y0 = getattr(s, key)
                y1 = index.get(s.timestamp + timedelta(minutes=horizon))
                if y0 is not None and y1 is not None and getattr(y1, key) is not None:
                    # data evaluated on holdout days only, forecast origin in test set
                    errors.append(y0 - getattr(y1, key))
            baseline[f"{key}@{horizon}min"] = _metrics(errors)
    return {
        "schema": "bloomml-external-greenhouse-audit-v1",
        "source": SOURCE_ID, "source_url": SOURCE_URL,
        "site": site.name, "relative_source_file": site.source_path,
        "source_sha256": sha.hexdigest(), "license": DATA_LICENSE,
        "provenance": "measured_external_processed_not_own_growbox",
        "clock_basis": CLOCK_BASIS, "timezone_confirmed": False,
        "sensor_truth": "measured_in_original_greenhouse_processed_15min",
        "actuator_truth": "control_status_or_setpoint_not_physical_airflow_confirmation",
        "physical_model_calibrated": False,
        "training_approval": "NOT_APPROVED_EXTERNAL_DOMAIN",
        "rows": rows, "first_source_time": str(observed[0].timestamp),
        "last_source_time": str(observed[-1].timestamp),
        "cadence_intervals_s": gap_counts,
        "completeness_fraction": completeness,
        "invalid_numeric_counts": invalid_counts,
        "auxiliary_nonmissing_counts": extra_counts,
        "actuator_nonmissing_counts": actuator_counts,
        "holdout_cutoff_local_naive_day": str(cutoff_date),
        "holdout_source_rows": len(test_sources),
        "baseline": baseline,
        "unsupported_horizons_min": [1, 5],
        "warnings": (
            ["RH coverage <80% — do not claim reliable RH model validation"]
            if completeness["relative_humidity_pct"] < .8 else []
        ),
    }

def main() -> int:
    parser = argparse.ArgumentParser(description="Audit a pinned external greenhouse CSV + held-out persistence")
    parser.add_argument("--file", type=Path, required=True)
    parser.add_argument("--site", choices=sorted(SITES), required=True)
    parser.add_argument("--expected-sha256", default=None)
    parser.add_argument("--output", type=Path, default=None)
    args = parser.parse_args()
    try:
        result = evaluate_file(args.file, args.site, expected_sha256=args.expected_sha256)
    except (ValueError, OSError, csv.Error) as exc:
        print(json.dumps({"status": "REJECTED", "reason": str(exc)}, ensure_ascii=False))
        return 2
    payload = json.dumps(result, indent=2, ensure_ascii=False, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(payload, encoding="utf-8")
    else:
        print(payload)
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
