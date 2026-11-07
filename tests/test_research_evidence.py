"""Verify numerical evidence cannot silently become a physical validation claim."""
from copy import deepcopy
from pathlib import Path

import pytest

from tools.ml.research_evidence import audit_stress_manifest, audit_module_boundaries


def sample():
    gpu = {"device": "cuda:0", "gpu_name": "Tesla T4", "scenarios": 6,
           "total_scenario_ticks": 120, "total_euler_substeps": 1000,
           "seconds": 2., "peak_cuda_torch_bytes": 123456,
           "pilot_cpu_scalar_count": 2,
           "pilot_cpu_cuda_max_absolute_error_by_field":
               {"air_temperature_c": 1e-10, "air_vapor_kg": 1e-16},
           "bounds": {"air_temperature_c_min": 9, "air_temperature_c_max": 35,
                      "relative_humidity_pct_min": 6, "relative_humidity_pct_max": 100,
                      "co2_ppm_min": 350, "co2_ppm_max": 4000,
                      "max_water_residual_kg": 1e-12,
                      "scenarios_rh_below_5pct": 0, "scenarios_co2_above_2000ppm": 2}}
    another = deepcopy(gpu)
    another["device"] = "cuda:1"
    return {"schema": "bloomml-r4-dual-t4-stress-v1",
            "source_commit": "a" * 40,
            "simulation_backend": "torch_cuda_batched_euler_r4",
            "synthetic_uncalibrated": True,
            "physical_validation_with_real_data": False,
            "devices": [gpu, another], "scenarios_total": 12, "ticks": 20,
            "dt_s": 10., "total_scenario_ticks": 240, "failed_physical_bounds": 0}


def test_numerical_evidence_never_claims_real_calibration():
    result = audit_stress_manifest(sample())
    assert result["status"] == "NUMERICAL_EVIDENCE_PASS"
    assert result["real_world_fidelity"] == "UNKNOWN"
    assert result["calibration_status"] == "NOT_EVALUATED"
    assert result["scenario_ticks"] == 240
    assert len(result["warnings"]) == 2


@pytest.mark.parametrize("field,value", [
    ("simulation_backend", "numpy_cpu"),
    ("synthetic_uncalibrated", False),
    ("physical_validation_with_real_data", True),
    ("source_commit", "bad-sha"),
    ("scenarios_total", 13),
    ("failed_physical_bounds", 1),
])
def test_rejects_false_claims_or_damaged_provenance(field, value):
    data = sample()
    data[field] = value
    with pytest.raises(ValueError):
        audit_stress_manifest(data)


def test_rejects_duplicate_cuda_or_mismatched_numbers():
    data = sample()
    data["devices"][1]["device"] = "cuda:0"
    with pytest.raises(ValueError):
        audit_stress_manifest(data)
    data = sample()
    data["devices"][1]["total_scenario_ticks"] += 1
    with pytest.raises(ValueError):
        audit_stress_manifest(data)


def test_rejects_nan_and_humidity_outside_range():
    data = sample()
    data["devices"][1]["bounds"]["relative_humidity_pct_min"] = float("nan")
    with pytest.raises(ValueError):
        audit_stress_manifest(data)
    data = sample()
    data["devices"][0]["bounds"]["relative_humidity_pct_max"] = 120
    with pytest.raises(ValueError):
        audit_stress_manifest(data)


def test_rejects_mass_balance_failure_and_parity_error():
    data = sample()
    data["devices"][0]["bounds"]["max_water_residual_kg"] = 1e-4
    with pytest.raises(ValueError):
        audit_stress_manifest(data)
    data = sample()
    data["devices"][1]["pilot_cpu_cuda_max_absolute_error_by_field"]["air_temperature_c"] = .01
    with pytest.raises(ValueError):
        audit_stress_manifest(data)


def test_research_modules_remain_isolated_from_production_and_remote_io():
    root = Path(__file__).resolve().parent.parent
    report = audit_module_boundaries(root)
    assert report["status"] == "DEPENDENCY_BOUNDARIES_PASS"
    assert len(report["modules"]) == 6
    assert any(m["name"] == "growbox_dynamics.py" and m["review_large_file"]
               for m in report["modules"])


def test_module_boundary_rejects_forbidden_import(tmp_path):
    module_dir = tmp_path / "tools" / "ml" / "physics"
    module_dir.mkdir(parents=True)
    from tools.ml.research_evidence import PHYSICS_MODULES
    for name in PHYSICS_MODULES:
        (module_dir / name).write_text("import math\n")
    (module_dir / PHYSICS_MODULES[0]).write_text("import serial\n")
    with pytest.raises(ValueError, match="pure physics boundary"):
        audit_module_boundaries(tmp_path)
