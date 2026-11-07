"""Read-only numerical evidence audit; no CUDA, firmware or Kaggle API needed.

A saved manifest is *reported evidence*: corroborate source_commit against Git
and the private Kaggle version before claiming independent verification.
"""
from __future__ import annotations

import argparse
import ast
import json
import math
from pathlib import Path
import re

R4_SCHEMA = "bloomml-r4-dual-t4-stress-v1"
PHYSICS_MODULES = ("growbox_dynamics.py", "growbox_psychrometrics.py",
                   "growbox_convection.py", "growbox_phase.py",
                   "growbox_tensor.py", "growbox_measurements.py")
BLOCKED_IMPORT_ROOTS = frozenset(("src", "lib", "serial", "platformio", "requests",
                                 "kaggle", "subprocess", "socket", "idf_component"))


def _nonnegative_finite(value: object, field: str) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ValueError(f"{field}: expected finite nonnegative number")
    number = float(value)
    if not math.isfinite(number) or number < 0:
        raise ValueError(f"{field}: expected finite nonnegative number")
    return number


def audit_stress_manifest(manifest: dict) -> dict:
    """Fail closed on R4 numerical evidence; warn on extreme agronomic outputs."""
    if not isinstance(manifest, dict) or manifest.get("schema") != R4_SCHEMA:
        raise ValueError("wrong stress evidence schema")
    sha = manifest.get("source_commit")
    if not isinstance(sha, str) or not re.fullmatch("[0-9a-f]{40}", sha):
        raise ValueError("source_commit must be a 40-character Git SHA")
    if manifest.get("simulation_backend") != "torch_cuda_batched_euler_r4":
        raise ValueError("simulation backend is not the verified R4 CUDA backend")
    if manifest.get("synthetic_uncalibrated") is not True:
        raise ValueError("synthetic uncertainty label missing")
    if manifest.get("physical_validation_with_real_data") is not False:
        raise ValueError("unsupported claim of empirical validation")
    devices = manifest.get("devices")
    if not isinstance(devices, list) or len(devices) != 2:
        raise ValueError("both independent CUDA devices are required")
    total = _nonnegative_finite(manifest.get("scenarios_total"), "scenarios_total")
    ticks = _nonnegative_finite(manifest.get("ticks"), "ticks")
    dt_s = _nonnegative_finite(manifest.get("dt_s"), "dt_s")
    declared_ticks = _nonnegative_finite(manifest.get("total_scenario_ticks"), "total_scenario_ticks")
    if not (total > 0 and ticks > 0 and dt_s > 0 and total == int(total)
            and ticks == int(ticks) and total * ticks == declared_ticks):
        raise ValueError("scenario totals or experiment duration disagree")
    if manifest.get("failed_physical_bounds") != 0:
        raise ValueError("physical bounds were violated")
    fields = ("air_temperature_c_min", "air_temperature_c_max",
              "relative_humidity_pct_min", "relative_humidity_pct_max",
              "co2_ppm_min", "co2_ppm_max", "max_water_residual_kg",
              "scenarios_rh_below_5pct", "scenarios_co2_above_2000ppm")
    summaries, warnings = [], []
    sum_count = sum_ticks = 0
    for index, gpu in enumerate(devices):
        if not isinstance(gpu, dict) or gpu.get("device") != f"cuda:{index}":
            raise ValueError("unexpected CUDA index (no CPU fallback/duplication)")
        if "T4" not in str(gpu.get("gpu_name", "")):
            raise ValueError("GPU is not the declared T4 accelerator")
        count = _nonnegative_finite(gpu.get("scenarios"), "gpu.scenarios")
        work = _nonnegative_finite(gpu.get("total_scenario_ticks"), "gpu.total_scenario_ticks")
        euler = _nonnegative_finite(gpu.get("total_euler_substeps"), "gpu.total_euler_substeps")
        seconds = _nonnegative_finite(gpu.get("seconds"), "gpu.seconds")
        _nonnegative_finite(gpu.get("peak_cuda_torch_bytes"), "gpu.allocated_bytes")
        pilot = _nonnegative_finite(gpu.get("pilot_cpu_scalar_count"), "gpu.pilot_count")
        if not (count > 0 and count == int(count) and pilot > 0 and pilot <= count
                and work == count * ticks and euler >= work and seconds > 0):
            raise ValueError("invalid GPU workload or CPU parity sample")
        parity = gpu.get("pilot_cpu_cuda_max_absolute_error_by_field")
        if not isinstance(parity, dict) or not parity:
            raise ValueError("CPU-CUDA parity metrics missing")
        max_parity = max(_nonnegative_finite(x, f"parity.{name}") for name, x in parity.items())
        if max_parity > 2e-6:
            raise ValueError("CPU-CUDA numerical parity mismatch")
        bounds = gpu.get("bounds")
        if not isinstance(bounds, dict) or any(f not in bounds for f in fields):
            raise ValueError("missing physics observability metrics")
        metrics = {f: _nonnegative_finite(bounds[f], f"bounds.{f}") for f in fields
                   if f not in ("air_temperature_c_min", "air_temperature_c_max")}
        for f in ("air_temperature_c_min", "air_temperature_c_max"):
            value = bounds[f]
            if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
                raise ValueError(f"bounds.{f} must be finite")
            metrics[f] = float(value)
        if not (-40 <= metrics["air_temperature_c_min"] <= metrics["air_temperature_c_max"] <= 80
                and metrics["relative_humidity_pct_min"] <= metrics["relative_humidity_pct_max"] <= 100.0001
                and metrics["co2_ppm_min"] <= metrics["co2_ppm_max"] <= 100_000
                and metrics["max_water_residual_kg"] <= 1e-8
                and metrics["scenarios_rh_below_5pct"] <= count
                and metrics["scenarios_co2_above_2000ppm"] <= count):
            raise ValueError("physics bounds inconsistent or water conservation failed")
        if metrics["scenarios_rh_below_5pct"]:
            warnings.append(f'cuda:{index}: {int(metrics["scenarios_rh_below_5pct"])} ultra-dry scenarios (<5% RH)')
        if metrics["scenarios_co2_above_2000ppm"]:
            warnings.append(f'cuda:{index}: {int(metrics["scenarios_co2_above_2000ppm"])} scenarios above 2000 ppm CO2')
        summaries.append({"device": gpu["device"], "scenarios": int(count),
                          "scenario_ticks": int(work), "euler_substeps": int(euler),
                          "max_cpu_cuda_absolute_error": max_parity,
                          "max_water_residual_kg": metrics["max_water_residual_kg"],
                          "seconds": seconds})
        sum_count += int(count)
        sum_ticks += int(work)
    if sum_count != total or sum_ticks != declared_ticks:
        raise ValueError("GPU workloads do not sum to declared experiment totals")
    return {"status": "NUMERICAL_EVIDENCE_PASS", "calibration_status": "NOT_EVALUATED",
            "real_world_fidelity": "UNKNOWN", "source_commit": sha,
            "scenarios_total": sum_count, "scenario_ticks": sum_ticks,
            "devices": summaries, "warnings": warnings,
            "interpretation": "GPU parity and physical bounds, not real growbox accuracy"}


def audit_module_boundaries(root: Path) -> dict:
    """Static research-domain dependency check, without changing the code."""
    modules = root / "tools" / "ml" / "physics"
    details = []
    for name in PHYSICS_MODULES:
        path = modules / name
        source = path.read_text(encoding="utf-8")
        tree = ast.parse(source, filename=str(path))
        imports = []
        for node in ast.walk(tree):
            if isinstance(node, ast.Import):
                imports.extend(alias.name for alias in node.names)
            elif isinstance(node, ast.ImportFrom) and node.level == 0:
                imports.append(node.module or "")
        invalid = sorted({name for name in imports if name.split(".")[0] in BLOCKED_IMPORT_ROOTS})
        if invalid:
            raise ValueError(f"{path.name} breaches pure physics boundary: {invalid}")
        details.append({"name": name, "lines": len(source.splitlines()),
                        "separation": "PASS", "review_large_file": len(source.splitlines()) >= 500})
    return {"status": "DEPENDENCY_BOUNDARIES_PASS", "modules": details}


def main() -> int:
    parser = argparse.ArgumentParser(description="Read-only BloomML Kaggle numerical evidence audit")
    parser.add_argument("manifest", type=Path)
    parser.add_argument("--root", type=Path, help="Optional BloomML source for import-boundary audit")
    args = parser.parse_args()
    try:
        report = audit_stress_manifest(json.loads(args.manifest.read_text(encoding="utf-8")))
        if args.root:
            report["architecture"] = audit_module_boundaries(args.root)
    except (ValueError, KeyError, OSError, json.JSONDecodeError) as exc:
        print(json.dumps({"status": "REJECTED", "reason": str(exc)}, sort_keys=True))
        return 2
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
