"""Offline importer for BloomML growbox-log-v3 GBLOG/*.JL captures.

The captured Stage27 format is distinct from climate-runtime-trace.v1.
This tool never infers physical actuator truth from controller requests.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
from collections import Counter
from pathlib import Path
from typing import Any

SCHEMA = "growbox-log-v3"
ACTION_NAMES = (
    "heater", "cooler", "exhaust_fan", "humidifier", "dehumidifier", "co2_doser",
)


def _number(value: object, *, low: float, high: float) -> float:
    if isinstance(value, bool) or not isinstance(value, int | float):
        raise ValueError("not a number")
    number = float(value)
    if not math.isfinite(number) or not low <= number <= high:
        raise ValueError("non-finite or out of plausible range")
    return number


def _nonnegative_int(value: object) -> int:
    if type(value) is not int or value < 0:
        raise ValueError("expected a nonnegative integer")
    return value


def _array(value: object, length: int, name: str) -> list[Any]:
    if not isinstance(value, list) or len(value) != length:
        raise ValueError(f"{name} must have {length} entries")
    return value


def _sensor(value: object, *, name: str, stale_after_ms: int) -> dict[str, object] | None:
    data = _array(value, 6 if name == "scd" else 4, name)
    sampled = _number(data[0], low=0, high=1) == 1
    if name == "scd":
        available = _number(data[1], low=0, high=1) == 1
        temp_index, humidity_index, age_index = 2, 3, 5
    else:
        available = True
        temp_index, humidity_index, age_index = 1, 2, 3
    age = _nonnegative_int(data[age_index])
    if not sampled or not available or age > stale_after_ms:
        return None
    output: dict[str, object] = {
        "temperature_c": _number(data[temp_index], low=-20.0, high=70.0),
        "humidity_pct": _number(data[humidity_index], low=0.0, high=100.0),
        "age_ms": age,
    }
    if name == "scd":
        output["co2_ppm"] = _number(data[4], low=250.0, high=5000.0)
    return output


def _fingerprint(path: Path) -> str:
    sha = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            sha.update(block)
    return sha.hexdigest()


def _iter_sources(source: Path) -> list[Path]:
    if source.is_file():
        if source.suffix.lower() != ".jl":
            raise ValueError("source file must have a .JL extension")
        return [source]
    if not source.is_dir():
        raise FileNotFoundError(source)
    paths = sorted(
        (path for path in source.rglob("*") if path.is_file() and path.suffix.lower() == ".jl"),
        key=lambda path: str(path),
    )
    if not paths:
        raise ValueError("no .JL files found")
    return paths


def _write_json_atomic(path: Path, data: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_name(path.name + ".tmp")
    try:
        tmp.write_text(
            json.dumps(data, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
        os.replace(tmp, path)
    finally:
        tmp.unlink(missing_ok=True)


def import_file(source: Path, destination: Path, *, stale_after_ms: int = 30000) -> dict[str, object]:
    """Read SD file and emit research observations; never imply calibration readiness."""
    if stale_after_ms < 0:
        raise ValueError("stale_after_ms must be nonnegative")
    digest = _fingerprint(source)
    output_path = destination / f"{source.stem}-{digest[:16]}.ndjson"
    report: dict[str, object] = {
        "source": str(source),
        "sha256": digest,
        "format": SCHEMA,
        "output": str(output_path),
        "accepted": 0,
        "rejected": 0,
        "health_records": 0,
        "rejection_reasons": {},
        "calibration_ready": False,
        "missing_context": [
            "outdoor temperature/humidity with provenance",
            "physical actuator confirmation and power/airflow characteristics",
        ],
    }
    reasons: Counter[str] = Counter()
    session: dict[str, object] | None = None
    previous_uptime: int | None = None
    previous_unix: int | None = None
    previous_raw_uptime: int | None = None
    accepted = 0
    destination.mkdir(parents=True, exist_ok=True)
    temp = output_path.with_name(output_path.name + ".tmp")
    try:
        with source.open("r", encoding="utf-8-sig") as stream, temp.open(
            "w", encoding="utf-8"
        ) as output:
            for line_no, raw in enumerate(stream, start=1):
                if not raw.strip():
                    continue
                try:
                    def reject_constant(token: str) -> None:
                        raise ValueError(f"non-finite JSON constant {token}")

                    record = json.loads(raw, parse_constant=reject_constant)
                    if not isinstance(record, dict):
                        raise ValueError("record is not an object")
                    kind = record.get("t")
                    if kind == "session":
                        if record.get("schema") != SCHEMA:
                            raise ValueError("unsupported schema")
                        if session is not None:
                            raise ValueError("multiple session headers in one file")
                        sid = record.get("sid")
                        if not isinstance(sid, str) or not sid:
                            raise ValueError("session ID missing")
                        session = {
                            "id": sid,
                            "firmware_sha": str(record.get("fw", "unknown")),
                            "backend": str(record.get("backend", "unknown")),
                            "rtc_trusted_at_start": bool(record.get("rtc", 0)),
                            "source_sha256": digest,
                        }
                        continue
                    if kind == "h":
                        report["health_records"] = int(report["health_records"]) + 1
                        continue
                    if kind != "s":
                        raise ValueError("unknown record type")
                    if session is None:
                        raise ValueError("sample before session header")
                    if record.get("v") != 3:
                        raise ValueError("unsupported sample version")
                    uptime = _nonnegative_int(record.get("u"))
                    if previous_raw_uptime is not None and uptime <= previous_raw_uptime:
                        raise ValueError("non-monotonic uptime")
                    previous_raw_uptime = uptime
                    unix = _nonnegative_int(record.get("x"))
                    scd = _sensor(record.get("scd"), name="scd", stale_after_ms=stale_after_ms)
                    if scd is None:
                        raise ValueError("SCD41 unavailable, not sampled, or stale")
                    actions = _array(record.get("c"), 10, "c")
                    applied = {
                        name: _number(value, low=0.0, high=1.0)
                        for name, value in zip(ACTION_NAMES, actions[4:], strict=True)
                    }
                    _nonnegative_int(actions[0])
                    _nonnegative_int(actions[1])
                    timestamp = unix if session["rtc_trusted_at_start"] and unix > 0 else None
                    if timestamp is not None and previous_unix is not None and timestamp < previous_unix:
                        raise ValueError("non-monotonic wall time")
                    if previous_uptime is not None and uptime <= previous_uptime:
                        raise ValueError("non-monotonic accepted uptime")
                    tp = _sensor(record.get("tp"), name="tp", stale_after_ms=stale_after_ms)
                    xm = _sensor(record.get("xm"), name="xm", stale_after_ms=stale_after_ms)
                    row: dict[str, object] = {
                        "source": {"sha256": digest, "filename": source.name, "line": line_no},
                        "session": session,
                        "uptime_ms": uptime,
                        "unix_time_s": timestamp,
                        "inside_scd41": scd,
                        "auxiliary_tp": tp,
                        "auxiliary_xiaomi": xm,
                        "controller_applied": applied,
                        "actuation_evidence": "controller decision; NOT physical output confirmation",
                        "physical_output_telemetry": record.get("out"),
                    }
                    output.write(
                        json.dumps(row, ensure_ascii=False, allow_nan=False, separators=(",", ":"))
                    )
                    output.write("\n")
                    accepted += 1
                    previous_uptime = uptime
                    if timestamp is not None:
                        previous_unix = timestamp
                except (ValueError, TypeError, KeyError, json.JSONDecodeError) as exc:
                    reasons[str(exc).split(":", 1)[0][:120]] += 1
    finally:
        if session is None:
            temp.unlink(missing_ok=True)
    report["accepted"] = accepted
    report["rejected"] = sum(reasons.values())
    report["rejection_reasons"] = dict(sorted(reasons.items()))
    report["session"] = session
    if session is None:
        report["error"] = "no valid session header"
        report["output"] = None
        return report
    os.replace(temp, output_path)
    return report


def import_capture(
    source: Path, destination: Path, *, stale_after_ms: int = 30000
) -> dict[str, object]:
    """Batch-import SD captures; write a deterministic, provenance-rich manifest."""
    reports = [
        import_file(path, destination, stale_after_ms=stale_after_ms)
        for path in _iter_sources(source)
    ]
    manifest: dict[str, object] = {
        "schema": "bloomml-stage27-import-v1",
        "sources": reports,
        "accepted": sum(int(r["accepted"]) for r in reports),
        "rejected": sum(int(r["rejected"]) for r in reports),
        "calibration_ready": False,
    }
    _write_json_atomic(destination / "import_manifest.json", manifest)
    return manifest


def main() -> int:
    parser = argparse.ArgumentParser(description="Offline BloomML SD GBLOG import (.JL to NDJSON)")
    parser.add_argument("source", type=Path, help=".JL file, GBLOG directory or SD card root")
    parser.add_argument("--out", type=Path, default=Path("data/stage27-import"))
    parser.add_argument("--stale-after-ms", type=int, default=30000)
    args = parser.parse_args()
    try:
        manifest = import_capture(args.source, args.out, stale_after_ms=args.stale_after_ms)
    except (ValueError, OSError) as exc:
        parser.error(str(exc))
    print(json.dumps({
        "accepted": manifest["accepted"],
        "rejected": manifest["rejected"],
        "manifest": str(args.out / "import_manifest.json"),
    }, ensure_ascii=False))
    return 0 if manifest["accepted"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
