import json
from pathlib import Path

from tools.ml.physics.run_thermodynamic_sweep import sweep


def test_deterministic_reproducible_golden_trajectories(tmp_path: Path) -> None:
    first = tmp_path / "a.ndjson"
    second = tmp_path / "b.ndjson"
    a = sweep(scenarios=2, steps=20, seed=13, trajectories=first)
    b = sweep(scenarios=2, steps=20, seed=13, trajectories=second)
    assert a == b
    assert first.read_bytes() == second.read_bytes()
    assert a["total_steps"] == 40
    assert not a["ready_for_hardware"]
    assert not a["ready_for_calibrated_training"]
    assert a["max_water_residual_kg"] < 1e-8
    row = json.loads(first.read_text().splitlines()[0])
    assert row["origin"] == "synthetic_uncalibrated"
    assert row["ppfd_umol_m2_s"] > 0


def test_different_seeds_change_golden_digest():
    a = sweep(scenarios=1, steps=5, seed=3)
    b = sweep(scenarios=1, steps=5, seed=4)
    assert a["sha256"] != b["sha256"]
