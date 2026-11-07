"""R2 scientific gating: CPU oracle and fail-closed CUDA preflight."""
import math
import pytest

torch=pytest.importorskip('torch')
from tools.ml.kaggle_gpu_r2_runner import cases, reference, parity, run
from tools.ml.physics.growbox_tensor import simulate_batch


def test_deterministic_cases_and_scalar_tensor_parity():
    p,s,f=cases(15,1729)
    gold=reference(p,s,f,dt_s=2,ticks=8)
    batch=simulate_batch(p,s,f,dt_s=2,ticks=8,device='cpu')
    errors=parity(gold,batch.states)
    assert max(errors.values()) < 1e-8
    assert max(map(abs,batch.water_residuals_kg))<1e-9
    assert cases(15,1729)==(p,s,f)


def test_parity_rejects_corrupt_gpu_values():
    from dataclasses import replace
    p,s,f=cases(1,60)
    result=reference(p,s,f,dt_s=2,ticks=2)
    with pytest.raises(AssertionError,match='temperature'):
        parity(result,[replace(result[0],air_temperature_c=result[0].air_temperature_c+0.001)])
    with pytest.raises(AssertionError,match='count'):
        parity(result,[])


def test_dual_gpu_gate_refuses_host_only(tmp_path):
    if torch.cuda.is_available() and torch.cuda.device_count()>=2:
        pytest.skip('actual two GPU hardware: run dedicated Kaggle integration instead')
    with pytest.raises(RuntimeError, match='two real independent CUDA GPUs'):
        run(output_dir=tmp_path,source_dir=tmp_path)
    assert not (tmp_path/'r2_gpu_physics_manifest.json').exists()
