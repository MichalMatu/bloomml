"""Stage27 SD importer tests (synthetic captures only, no hardware)."""

from __future__ import annotations

import json
from pathlib import Path

from tools.ml.stage27_sd_import import import_capture, import_file


def session(**changes: object) -> dict[str, object]:
    return {
        "t": "session", "schema": "growbox-log-v3", "sid": "001a0001", "fw": "abc123",
        "rtc": 1, "backend": "sd", **changes,
    }


def sample(u: int = 10000, **changes: object) -> dict[str, object]:
    return {
        "t": "s", "v": 3, "u": u, "x": 1793736000 + u // 1000,
        "scd": [1, 1, 24.0, 60.0, 700.0, 1000],
        "tp": [0, 0.0, 0.0, 0], "xm": [0, 0.0, 0.0, 0],
        "c": [0, 0, 0, 0, 0.2, 0.0, 0.3, 0.5, 0.0, 0.0],
        "out": {"v": 2, "ep": []}, **changes,
    }


def write(path: Path, records: list[dict[str, object]]) -> None:
    path.write_text(
        "\n".join(json.dumps(record) for record in records) + "\n", encoding="utf-8"
    )


def test_import_preserves_provenance_and_control_vs_physical_boundary(tmp_path: Path) -> None:
    source = tmp_path / "GBLOG" / "01.JL"
    source.parent.mkdir()
    write(source, [session(), sample(), sample(20000)])
    destination = tmp_path / "import"
    result = import_capture(source.parent, destination)
    assert result["accepted"] == 2
    assert result["rejected"] == 0
    assert not result["calibration_ready"]
    report = result["sources"][0]
    rows = [
        json.loads(line) for line in Path(str(report["output"])).read_text().splitlines()
    ]
    assert rows[0]["inside_scd41"]["temperature_c"] == 24.0
    assert rows[0]["controller_applied"]["exhaust_fan"] == 0.3
    assert "NOT physical" in rows[0]["actuation_evidence"]
    assert rows[0]["source"]["sha256"] == report["sha256"]
    again = import_capture(source.parent, destination)
    assert again["sources"][0]["output"] == report["output"]
    assert len(list(destination.glob("*.ndjson"))) == 1


def test_import_rejects_invalid_stale_and_nonmonotonic_records(tmp_path: Path) -> None:
    source = tmp_path / "2.JL"
    write(source, [
        session(), sample(),
        sample(20000, scd=[1, 1, 24.0, 60.0, 800.0, 50000]),
        sample(15000),
        sample(30000, scd=[1, 1, 24.0, 160.0, 800.0, 0]),
        sample(40000),
    ])
    report = import_file(source, tmp_path / "out")
    assert report["accepted"] == 2
    assert report["rejected"] == 3


def test_import_handles_untrusted_clock_and_missing_session(tmp_path: Path) -> None:
    source = tmp_path / "no-clock.JL"
    write(source, [session(rtc=0), sample()])
    report = import_file(source, tmp_path / "out")
    row = json.loads(Path(str(report["output"])).read_text().splitlines()[0])
    assert row["unix_time_s"] is None

    invalid = tmp_path / "broken.JL"
    write(invalid, [sample()])
    report = import_file(invalid, tmp_path / "out")
    assert report["accepted"] == 0
    assert report["error"] == "no valid session header"
    assert report["output"] is None


def test_health_records_are_not_samples(tmp_path: Path) -> None:
    source = tmp_path / "health.JL"
    write(source, [session(), {"t": "h", "v": 3, "u": 1000}, sample()])
    report = import_file(source, tmp_path / "out")
    assert report["health_records"] == 1
    assert report["accepted"] == 1
