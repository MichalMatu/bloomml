"""Kaggle-side reproducibility boundary for BloomML research physics.

This module does NOT claim that the NumPy physics solver runs on CUDA.
It verifies both CUDA devices and emits an exact source/provenance manifest.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from tools.ml.physics.run_thermodynamic_sweep import sweep

SCHEMA = "bloomml-kaggle-run-v1"


def git_sha(repo: Path) -> str:
    return subprocess.check_output(
        ["git", "-C", str(repo), "rev-parse", "--verify", "HEAD"],
        text=True,
    ).strip()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def gpu_probe(*, required: int = 2) -> dict[str, object]:
    """Actually execute a tiny CUDA calculation on each discovered GPU."""
    if required < 1:
        raise ValueError("required must be >= 1")
    try:
        import torch
    except ImportError as exc:
        raise RuntimeError("PyTorch not installed: Kaggle GPU environment required") from exc
    if not torch.cuda.is_available():
        raise RuntimeError("CUDA unavailable: select GPU T4 x2 accelerator in Kaggle")
    count = torch.cuda.device_count()
    if count < required:
        raise RuntimeError(f"expected >= {required} independent CUDA GPUs, found {count}")
    devices = []
    for index in range(required):
        device = torch.device(f"cuda:{index}")
        # Operate on both cards; not a simulation/ML training workload.
        a = torch.ones((128, 128), device=device, dtype=torch.float32)
        b = torch.full((128, 128), 2.0, device=device, dtype=torch.float32)
        c = a @ b
        torch.cuda.synchronize(index)
        value = float(c[0, 0].item())
        if abs(value - 256.0) > 1e-2:
            raise RuntimeError(f"CUDA device {index} arithmetic check failed: {value}")
        devices.append({
            "cuda_index": index, "name": torch.cuda.get_device_name(index),
            "smoke_matmul_value": value, "device": str(device),
        })
    return {
        "cuda_available": True,
        "device_count": count,
        "used_devices": devices,
        "torch_version": str(torch.__version__),
        "cuda_version": str(torch.version.cuda),
        "physics_on_gpu": False,
    }


def execute(*, output_dir: Path, scenarios: int, steps: int,
            seed: int, required_gpus: int, source_dir: Path) -> dict[str, object]:
    """GPU-preflight + deterministic CPU oracle, explicitly not GPU physics."""
    source_sha = git_sha(source_dir)
    gpu = gpu_probe(required=required_gpus)
    output_dir.mkdir(parents=True, exist_ok=True)
    trajectories = output_dir / "cpu_golden.ndjson"
    physics = sweep(scenarios=scenarios, steps=steps, seed=seed,
                    trajectories=trajectories)
    manifest = {
        "schema": SCHEMA,
        "repository": "MichalMatu/bloomml",
        "source_commit": source_sha,
        "source_branch_intent": "research/simulator-fidelity-kaggle-20261103",
        "execution": "kaggle",
        "python": sys.version.split()[0],
        "accelerator": gpu,
        "stage": "gpu_device_preflight_plus_cpu_oracle",
        "simulation_backend": "numpy_cpu",
        "gpu_physics_ready": False,
        "calibrated": False,
        "training_ready": False,
        "workflow": "offline_research_only",
        "seed": seed,
        "scenarios": scenarios,
        "steps_per_scenario": steps,
        "physics_summary": physics,
        "artifacts": {
            "cpu_golden.ndjson": {
                "sha256": sha256_file(trajectories),
                "bytes": trajectories.stat().st_size,
                "origin": "synthetic_uncalibrated",
            }
        },
    }
    # No secrets, private paths, tokens or raw sensor logs in manifest.
    manifest_path = output_dir / "kaggle_manifest.json"
    manifest_path.write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    return manifest


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Kaggle T4x2 preflight and reproducible CPU reference for BloomML"
    )
    parser.add_argument("--output", type=Path, default=Path("/kaggle/working/bloomml"))
    parser.add_argument("--source", type=Path, default=Path.cwd())
    parser.add_argument("--scenarios", type=int, default=4)
    parser.add_argument("--steps", type=int, default=120)
    parser.add_argument("--seed", type=int, default=1729)
    parser.add_argument("--require-gpus", type=int, default=2)
    args = parser.parse_args()
    manifest = execute(
        output_dir=args.output, source_dir=args.source,
        scenarios=args.scenarios, steps=args.steps,
        seed=args.seed, required_gpus=args.require_gpus,
    )
    print(json.dumps({
        "source_commit": manifest["source_commit"],
        "gpu_names": [x["name"] for x in manifest["accelerator"]["used_devices"]],
        "cpu_oracle_total_steps": manifest["physics_summary"]["total_steps"],
        "cpu_oracle_sha256": manifest["physics_summary"]["sha256"],
        "gpu_physics_ready": False,
        "manifest": str(args.output / "kaggle_manifest.json"),
    }, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
