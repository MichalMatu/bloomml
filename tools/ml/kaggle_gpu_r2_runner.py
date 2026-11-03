"""Real dual-CUDA-GPU physics pilot for Kaggle (never a CPU-only success).

The CPU scalar oracle remains the numerical reference. The Torch backend runs
thermo-hygro-CO2 physics on *both* separate CUDA devices, then saves limited
provenance. This is offline research, not calibrated control or production ML.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
from dataclasses import fields, replace
import json
import math
from pathlib import Path
import random
import subprocess
from time import perf_counter

from tools.ml.physics.growbox_dynamics import (
    GrowboxForcing, GrowboxGeometry, GrowboxLighting, GrowboxPhysics,
    GrowboxState, GrowboxThermodynamics,
)
from tools.ml.physics.growbox_tensor import simulate_batch

SCHEMA = 'bloomml-gpu-physics-r2-pilot-v1'


def cases(count: int, seed: int):
    """Heterogeneous but deliberately bounded synthetic comparison cases."""
    if not 1 <= count <= 100_000:
        raise ValueError('invalid scenario count')
    rng = random.Random(seed)
    params, states, forcings = [], [], []
    for i in range(count):
        volume = rng.uniform(0.30, 2.4)
        p = GrowboxPhysics(
            geometry=replace(GrowboxGeometry(), air_volume_m3=volume,
                             wall_heat_capacity_j_k=rng.uniform(75_000, 140_000),
                             canopy_area_m2=rng.uniform(.4, 1.2),
                             wall_air_conductance_w_k=rng.uniform(6, 19)),
            lighting=replace(GrowboxLighting(), rated_power_w=rng.uniform(55, 180)))
        t = rng.uniform(19, 28)
        initial = GrowboxState.initial(
            volume_m3=volume, air_temperature_c=t,
            rh_pct=rng.uniform(30, 75), co2_ppm=rng.uniform(400, 1200),
            wall_temperature_c=t+rng.uniform(-1.5,1.5),
            leaf_temperature_c=t+rng.uniform(-1.5,1.5),
            substrate_temperature_c=t+rng.uniform(-1.5,1.5),
            substrate_water_kg=rng.uniform(.3, 2.1))
        f = GrowboxForcing(
            outside_temperature_c=rng.uniform(12,29),
            outside_relative_humidity_pct=rng.uniform(25,80),
            lamp_dimmer=rng.random(), heater=rng.random()*.5,
            cooler=rng.random()*.3, exhaust_fan=rng.random(),
            circulation_fan=rng.random(), humidifier=rng.random()*.5,
            dehumidifier=rng.random()*.4, co2_doser=rng.random()*.4,
            measured_air_speed_m_s=(rng.random() if i%7==0 else None),
            measured_exhaust_m3_h=(rng.uniform(10,60) if i%9==0 else None),
        )
        params.append(p); states.append(initial); forcings.append(f)
    return params, states, forcings


def reference(params, states, forcings, *, dt_s: float, ticks: int):
    result=[]
    for p,s,f in zip(params,states,forcings):
        m=GrowboxThermodynamics(p,s)
        for _ in range(ticks):
            m.step(f,dt_s)
        result.append(m.state)
    return result


def parity(reference_states, tested_states, *, abs_tolerance: float=2e-7,
           relative_tolerance: float=2e-7) -> dict[str,float]:
    if len(reference_states)!=len(tested_states):
        raise AssertionError('scenario count mismatch')
    maximum={f.name:0.0 for f in fields(GrowboxState)}
    for cpu,gpu in zip(reference_states,tested_states):
        for f in fields(GrowboxState):
            a,b=getattr(cpu,f.name),getattr(gpu,f.name)
            if not math.isfinite(b) or not math.isclose(a,b,rel_tol=relative_tolerance,abs_tol=abs_tolerance):
                raise AssertionError(f'CPU/GPU state mismatch: {f.name} {a!r} != {b!r}')
            maximum[f.name]=max(maximum[f.name],abs(a-b))
    return maximum


def run(*, output_dir: Path, source_dir: Path, seed: int=1729,
        scenarios_per_gpu: int=256, ticks: int=8, dt_s: float=2.0,
        parity_per_gpu: int=8) -> dict:
    import torch
    if not torch.cuda.is_available() or torch.cuda.device_count()<2:
        raise RuntimeError('two real independent CUDA GPUs required; no CPU fallback')
    if parity_per_gpu <= 0 or scenarios_per_gpu < parity_per_gpu or ticks <= 0:
        raise ValueError('invalid batch size or ticks')
    source_sha = subprocess.check_output(['git','-C',str(source_dir),'rev-parse','HEAD'],text=True).strip()
    if len(source_sha)!=40 or any(c not in '0123456789abcdef' for c in source_sha):
        raise RuntimeError('invalid provenance commit')
    total_scenarios=2*scenarios_per_gpu
    params,states,forcings=cases(total_scenarios,seed)
    validation=[]

    def run_card(card_index: int):
        start_index=card_index*scenarios_per_gpu
        stop_index=start_index+scenarios_per_gpu
        p,s,f=(item[start_index:stop_index] for item in (params,states,forcings))
        n=parity_per_gpu
        scalar=reference(p[:n],s[:n],f[:n],dt_s=dt_s,ticks=ticks)
        pilot=simulate_batch(p[:n],s[:n],f[:n],dt_s=dt_s,ticks=ticks,
                             device=f'cuda:{card_index}',dtype=torch.float64)
        difference=parity(scalar,pilot.states)
        if max(abs(r) for r in pilot.water_residuals_kg)>1e-8:
            raise AssertionError('GPU pilot water mass residual too high')
        torch.cuda.synchronize(card_index)
        torch.cuda.reset_peak_memory_stats(card_index)
        tic=perf_counter()
        batch=simulate_batch(p,s,f,dt_s=dt_s,ticks=ticks,
                             device=f'cuda:{card_index}',dtype=torch.float64)
        torch.cuda.synchronize(card_index)
        wall=perf_counter()-tic
        if max(abs(r) for r in batch.water_residuals_kg)>1e-8:
            raise AssertionError('GPU batch water mass residual too high')
        return {
            'device':f'cuda:{card_index}',
            'gpu_name':torch.cuda.get_device_name(card_index),
            'dtype':'torch.float64',
            'pilot_scenarios':n,
            'pilot_max_absolute_error_by_field':difference,
            'pilot_max_water_residual_kg':max(map(abs,pilot.water_residuals_kg)),
            'batch_scenarios':len(batch.states),
            'substeps_per_tick_min':min(batch.substeps_per_tick),
            'substeps_per_tick_max':max(batch.substeps_per_tick),
            'batch_wall_seconds':wall,
            'batch_scenario_ticks_per_s':len(batch.states)*ticks/wall,
            'batch_peak_cuda_allocated_bytes':torch.cuda.max_memory_allocated(card_index),
            'batch_max_water_residual_kg':max(map(abs,batch.water_residuals_kg)),
        }

    # Two explicit device allocations; never rely on combined VRAM.
    with ThreadPoolExecutor(max_workers=2) as executor:
        future0=executor.submit(run_card,0)
        future1=executor.submit(run_card,1)
        validation=[future0.result(),future1.result()]
    manifest={
        'schema':SCHEMA,
        'repository':'MichalMatu/bloomml',
        'source_commit':source_sha,
        'source_branch_intent':'research/simulator-fidelity-kaggle-20261103',
        'execution':'kaggle',
        'simulation_backend':'torch_cuda_batched_euler_v1',
        'gpu_physics_ready':True,
        'meaning':'numerical_cuda_pilot_only_not_a_calibrated_digital_twin',
        'calibrated':False,
        'training_ready':False,
        'water_mass_conservation_tolerance_kg':1e-8,
        'parity_abs_tolerance':2e-7,
        'parity_rel_tolerance':2e-7,
        'dtype':'torch.float64',
        'seed':seed,'ticks':ticks,'time_step_s':dt_s,
        'scenarios_total':total_scenarios,
        'devices':validation,
        'torch_version':torch.__version__,
        'cuda_version':torch.version.cuda,
    }
    output_dir.mkdir(parents=True,exist_ok=True)
    (output_dir/'r2_gpu_physics_manifest.json').write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\n')
    return manifest


def main():
    parser=argparse.ArgumentParser(description='Run real dual-T4 physics pilot with CPU parity')
    parser.add_argument('--source',type=Path,default=Path.cwd())
    parser.add_argument('--output',type=Path,default=Path('/kaggle/working/bloomml-run'))
    parser.add_argument('--scenarios-per-gpu',type=int,default=256)
    parser.add_argument('--parity-per-gpu',type=int,default=8)
    parser.add_argument('--ticks',type=int,default=8)
    parser.add_argument('--dt-s',type=float,default=2.0)
    parser.add_argument('--seed',type=int,default=1729)
    args=parser.parse_args()
    summary=run(output_dir=args.output,source_dir=args.source,
                seed=args.seed,scenarios_per_gpu=args.scenarios_per_gpu,
                ticks=args.ticks,dt_s=args.dt_s,parity_per_gpu=args.parity_per_gpu)
    print(json.dumps({'source_commit':summary['source_commit'],
                      'gpu_physics_ready':summary['gpu_physics_ready'],
                      'scenarios_total':summary['scenarios_total'],
                      'devices':summary['devices']},indent=2))


if __name__=='__main__':
    main()
