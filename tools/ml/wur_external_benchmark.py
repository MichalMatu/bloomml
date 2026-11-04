"""Audit a *pinned Kaggle MIRROR* of WUR AGC 2019, without claiming original equivalence.

No internet/API access, GPU, firmware dependencies or raw data persistence.
All raw CSVs are read directly from a locally supplied ZIP; no extraction needed.
Timestamp %time is an Excel-like serial day with 5-minute quantization, NOT
verified UTC. Requested/realized controls MUST NOT become measured airflow.
"""
from __future__ import annotations

import argparse
from collections import Counter
import csv
from datetime import datetime, timedelta
from hashlib import sha256
from io import TextIOWrapper
import json
import math
from pathlib import Path
from zipfile import ZipFile, BadZipFile

MIRROR_ID = "piantic/autonomous-greenhouse-challengeagc-2nd-2019"
MIRROR_SHA256 = "c19d385e7f4731a77e59ebfd763fd49e35e7ac4128d0aaf41b5717709cd75a68"
ORIGINAL_DOI = "10.4121/uuid:88d22c60-21b3-4ea8-90db-20249a5be2a7"
LICENSE = "CC0-1.0"
SOURCE_CLOCK = "excel_serial_naive_5min_timezone_unverified"
TICK_SECONDS = 300
TICKS_PER_DAY = 288
SITES = ("AICU", "Automatoes", "Digilog", "IUACAAS", "Reference", "TheAutomators")
MEASURED_TARGETS = {
    "Tair": ("temperature_c", -40., 80.),
    "Rhair": ("relative_humidity_pct", 0., 100.),
    "CO2air": ("co2_ppm", 0., 100_000.),
    "Tot_PAR": ("reported_par_umol_m2_s", 0., 10_000.),
}
HORIZONS_MIN = (5, 15, 60)
METEO_KEYS = ("Tout", "Rhout", "PARout", "Iglob")
CONTROL_KEYS = ("VentLee", "Ventwind", "AssimLight", "co2_dos", "co2_sp",
                "co2_vip", "t_heat_sp", "t_heat_vip", "window_pos_lee_sp",
                "window_pos_lee_vip", "t_ventlee_vip", "t_ventwind_vip")
SOURCE_TYPES = {
    "Tair": "source_reported_climate_observation",
    "Rhair": "source_reported_climate_observation",
    "CO2air": "source_reported_climate_observation",
    "Tot_PAR": "source_reported_par_unverified_sensor_vs_derived",
}
EXCEL_EPOCH = datetime(1899, 12, 30)


def excel_tick(value: str) -> int:
    """Round source 5-decimal Excel day to exact five-minute index; no UTC claim."""
    try:
        raw = float(value.strip())
    except (ValueError, AttributeError) as exc:
        raise ValueError("invalid %time Excel serial day") from exc
    scaled = raw * TICKS_PER_DAY
    if not math.isfinite(scaled) or not 30000 * TICKS_PER_DAY <= scaled <= 70000 * TICKS_PER_DAY:
        raise ValueError("out-of-domain Excel timestamp")
    index = round(scaled)
    # 5 decimal places in source: tick quantization jitter <0.0015 tick
    if abs(scaled - index) > .02:
        raise ValueError("source timestamp is not five-minute aligned")
    return index


def tick_date(tick: int) -> str:
    return str((EXCEL_EPOCH + timedelta(seconds=tick * TICK_SECONDS)).date())


def numeric(raw: str | None, lo: float, hi: float) -> tuple[float | None, bool]:
    """Returns (value, invalid); missing NaN is not silently zeroed."""
    if raw is None or raw.strip().lower() in ("", "nan", "null", "none", "na"):
        return None, False
    try:
        value = float(raw)
    except ValueError:
        return None, True
    if not math.isfinite(value) or not lo <= value <= hi:
        return None, True
    return value, False


def baseline(errors: list[float]) -> dict:
    if not errors:
        return {"status": "NO_COMPLETE_PAIRS", "n": 0}
    n = len(errors)
    return {"status": "PERSISTENCE_ONLY", "n": n,
            "mae": sum(abs(x) for x in errors) / n,
            "rmse": math.sqrt(sum(x * x for x in errors) / n),
            "bias_pred_minus_actual": sum(errors) / n}


def audit_member(z: ZipFile, name: str, targets: dict[str, tuple[str, float, float]],
                 *, holdout_fraction: float, control_fields: tuple[str, ...] = ()) -> tuple[dict, dict[int, dict]]:
    if name not in z.namelist():
        raise ValueError("expected WUR source member missing: " + name)
    info = z.getinfo(name)
    if info.file_size > 30_000_000:
        raise ValueError("unexpected uncompressed CSV size")
    observations: dict[int, dict] = {}
    missing, invalid, measured_ranges, actuation = Counter(), Counter(), {}, Counter()
    previous = None
    first = last = None
    gaps = Counter()
    with z.open(name) as raw, TextIOWrapper(raw, encoding="utf-8-sig", errors="strict", newline="") as text:
        reader = csv.DictReader(text)
        columns = reader.fieldnames or []
        required = {"%time"} | set(targets)
        if len(set(columns)) != len(columns) or not required.issubset(columns):
            raise ValueError("WUR schema differs from pinned source: " + name)
        for row in reader:
            tick = excel_tick(row["%time"])
            if previous is not None:
                delta = tick - previous
                if delta <= 0:
                    raise ValueError("duplicate or reversed source tick")
                gaps[delta] += 1
            previous = tick
            if first is None:
                first = tick
            last = tick
            values = {}
            for col, (_, lo, hi) in targets.items():
                val, bad = numeric(row.get(col), lo, hi)
                if bad:
                    invalid[col] += 1
                if val is None:
                    missing[col] += 1
                else:
                    if col not in measured_ranges:
                        measured_ranges[col] = [val, val]
                    else:
                        measured_ranges[col][0] = min(measured_ranges[col][0], val)
                        measured_ranges[col][1] = max(measured_ranges[col][1], val)
                values[col] = val
            for col in control_fields:
                if col in columns:
                    # Control channels only: no physical airflow or watt confirmation.
                    raw_value = row.get(col)
                    if raw_value is not None and raw_value.strip().lower() not in ("", "nan", "none", "null"):
                        actuation[col] += 1
            observations[tick] = values
    if first is None or last is None or len(observations) < 100:
        raise ValueError("too few WUR records")
    days = sorted({tick_date(t) for t in observations})
    if len(days) < 10:
        raise ValueError("too few days for chronological holdout")
    test_cutoff = days[-max(1, math.ceil(len(days) * holdout_fraction))]
    heldout_ticks = [tick for tick in observations if tick_date(tick) >= test_cutoff]
    if len(heldout_ticks) < 100:
        raise ValueError("too few heldout rows")
    evaluation = {}
    for col, (label, _, _) in targets.items():
        for horizon in HORIZONS_MIN:
            step = horizon * 60 // TICK_SECONDS
            errors = []
            for tick in heldout_ticks:
                source_val = observations[tick][col]
                actual = observations.get(tick + step)
                if actual is not None and source_val is not None and actual[col] is not None:
                    errors.append(source_val - actual[col])
            evaluation[f"{label}@{horizon}min"] = baseline(errors)
    extra_warnings = []
    total = len(observations)
    cover = {targets[k][0]: (total - missing[k]) / total for k in targets}
    for label, ratio in cover.items():
        if ratio < .80:
            extra_warnings.append(f"{label} completeness below 80%")
    if gaps.get(1, 0) < total - 1:
        extra_warnings.append("source timestamp gaps, no interpolation")
    if any(invalid.values()):
        extra_warnings.append("invalid numeric values skipped, never imputed")
    summary = {
        "member": name, "rows": total, "source_columns": len(columns),
        "first_source_excel_serial_tick": first,
        "last_source_excel_serial_tick": last,
        "first_date_unverified_timezone": tick_date(first),
        "last_date_unverified_timezone": tick_date(last),
        "source_tick_deltas_5min": dict(gaps.most_common(10)),
        "missing_values": dict(missing),
        "invalid_numeric_values": dict(invalid),
        "coverage": cover, "observed_ranges": measured_ranges,
        "control_column_nonmissing": dict(actuation),
        "holdout_cutoff_naive_day": test_cutoff, "holdout_rows": len(heldout_ticks),
        "baseline": evaluation, "unsupported_horizons_min": [1],
        "warnings": extra_warnings,
    }
    return summary, observations


def audit_archive(source_zip: Path, *, expected_sha256: str = MIRROR_SHA256,
                  holdout_fraction: float = .2) -> dict:
    if not .05 <= holdout_fraction <= .5:
        raise ValueError("invalid chronological holdout fraction")
    if not source_zip.is_file():
        raise ValueError("source ZIP not found")
    h = sha256()
    with source_zip.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    digest = h.hexdigest()
    if expected_sha256 and digest != expected_sha256:
        raise ValueError("Kaggle mirror archive SHA256 mismatch")
    with ZipFile(source_zip) as z:
        if z.testzip() is not None:
            raise ValueError("corrupt archive CRC")
        weather, weather_index = audit_member(
            z, "Weather/Weather.csv",
            {x: (x, -80., 80.) if x == "Tout"
             else (x, 0., 100.) if x == "Rhout"
             else (x, 0., 10_000.) for x in METEO_KEYS},
            holdout_fraction=holdout_fraction)
        sites = []
        for site in SITES:
            name = site + "/GreenhouseClimate.csv"
            stats, data = audit_member(z, name, MEASURED_TARGETS,
                                       holdout_fraction=holdout_fraction,
                                       control_fields=CONTROL_KEYS)
            common = set(data).intersection(weather_index)
            stats["site"] = site
            stats["weather_matching_exact_timestamps"] = len(common)
            stats["weather_missing_exact_timestamps"] = len(data) - len(common)
            # The weather/actuator signals are NOT fed into persistence.
            sites.append(stats)
    total_rows = sum(x["rows"] for x in sites)
    return {
        "schema": "bloomml-wur-2019-mirror-benchmark-v1",
        "original_doi": ORIGINAL_DOI, "original_license": LICENSE,
        "mirror_ref": MIRROR_ID, "mirror_sha256": digest,
        "original_archive_byte_equivalence": "NOT_VERIFIED",
        "provenance": "measured_external_via_unverified_third_party_mirror",
        "clock_basis": SOURCE_CLOCK, "timezone_confirmed": False,
        "training_approval": "EXPLORATORY_ONLY_ORIGINAL_VERIFICATION_PENDING",
        "physical_growbox_calibration": False, "source_processed_raw_status": "NOT_INDEPENDENTLY_VERIFIED",
        "target_column_meanings": SOURCE_TYPES,
        "control_column_semantics": "requested_or_realized_setpoints_not_proven_physical_airflow",
        "split": "last_20_percent_source_naive_days_per_compartment",
        "leakage_guard": "persistence_uses_only_test_origin_current_value_and_future_real_target_for_scoring",
        "supported_horizons_minutes": list(HORIZONS_MIN),
        "total_climate_rows": total_rows,
        "weather": {k: v for k, v in weather.items() if k != "baseline"},
        "sites": sites,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="WUR CC0 mirror: audit 6 houses, weather and persistence")
    parser.add_argument("--zip", type=Path, required=True)
    parser.add_argument("--expected-sha256", default=MIRROR_SHA256)
    parser.add_argument("--output", type=Path, default=None)
    args = parser.parse_args()
    try:
        result = audit_archive(args.zip, expected_sha256=args.expected_sha256)
    except (ValueError, OSError, BadZipFile, csv.Error) as exc:
        print(json.dumps({"status": "REJECTED", "reason": str(exc)}, ensure_ascii=False))
        return 2
    payload = json.dumps(result, ensure_ascii=False, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(payload, encoding="utf-8")
    else:
        print(payload)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
