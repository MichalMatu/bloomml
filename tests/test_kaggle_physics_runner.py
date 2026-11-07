"""Offline contract checks for the Kaggle research handoff.

No Kaggle login or CUDA card required; GPU mock checks are NOT GPU evidence.
"""
from __future__ import annotations

import json
from pathlib import Path
from types import SimpleNamespace

import pytest

from tools.ml import kaggle_physics_runner as runner


def test_gpu_probe_fails_closed_without_required_cards(monkeypatch):
    import sys
    fake = SimpleNamespace(cuda=SimpleNamespace(is_available=lambda: False))
    monkeypatch.setitem(sys.modules, "torch", fake)
    with pytest.raises(RuntimeError, match="CUDA unavailable"):
        runner.gpu_probe(required=2)


def test_gpu_probe_fails_closed_when_only_one_card(monkeypatch):
    import sys
    fake = SimpleNamespace(cuda=SimpleNamespace(
        is_available=lambda: True, device_count=lambda: 1))
    monkeypatch.setitem(sys.modules, "torch", fake)
    with pytest.raises(RuntimeError, match="expected >= 2"):
        runner.gpu_probe(required=2)


def test_gpu_probe_performs_arithmetic_on_each_device(monkeypatch):
    import sys

    class Scalar:
        def item(self):
            return 256.0

    class Matrix:
        def __matmul__(self, other):
            return self

        def __getitem__(self, index):
            assert index == (0, 0)
            return Scalar()

    used = []
    cuda = SimpleNamespace(
        is_available=lambda: True,
        device_count=lambda: 2,
        synchronize=lambda idx: used.append(idx),
        get_device_name=lambda idx: f"T4-{idx}",
    )
    fake = SimpleNamespace(
        cuda=cuda,
        device=lambda value: value,
        float32="float32",
        ones=lambda *a, **kw: Matrix(),
        full=lambda *a, **kw: Matrix(),
        __version__="test",
        version=SimpleNamespace(cuda="test"),
    )
    monkeypatch.setitem(sys.modules, "torch", fake)
    result = runner.gpu_probe(required=2)
    assert used == [0, 1]
    assert [d["cuda_index"] for d in result["used_devices"]] == [0, 1]
    assert result["physics_on_gpu"] is False


def test_cpu_oracle_manifest_is_pinned_and_never_claims_gpu_physics(monkeypatch, tmp_path):
    sha = "a" * 40
    monkeypatch.setattr(runner, "git_sha", lambda source: sha)
    monkeypatch.setattr(runner, "gpu_probe", lambda required: {
        "cuda_available": True,
        "device_count": 2,
        "used_devices": [{"cuda_index": 0, "name": "T4"},
                         {"cuda_index": 1, "name": "T4"}],
        "physics_on_gpu": False,
    })

    def sweep_stub(*, scenarios, steps, seed, trajectories):
        trajectories.write_text('{"schema":"golden"}\n')
        return {"total_steps": scenarios * steps, "sha256": "physics-hash"}

    monkeypatch.setattr(runner, "sweep", sweep_stub)
    out = tmp_path / "results"
    m = runner.execute(
        output_dir=out, source_dir=tmp_path,
        scenarios=3, steps=4, seed=1729, required_gpus=2)
    assert m["source_commit"] == sha
    assert m["stage"] == "gpu_device_preflight_plus_cpu_oracle"
    assert m["simulation_backend"] == "numpy_cpu"
    assert not m["gpu_physics_ready"]
    assert not m["training_ready"]
    assert m["physics_summary"]["total_steps"] == 12
    assert m["artifacts"]["cpu_golden.ndjson"]["sha256"] == runner.sha256_file(
        out / "cpu_golden.ndjson")
    assert json.loads((out / "kaggle_manifest.json").read_text()) == m
