"""Heavy independent CUDA physics stress; synthetic numerical evidence only."""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
from dataclasses import replace
import json
import math
from pathlib import Path
import random
import subprocess
from time import perf_counter

from tools.ml.kaggle_gpu_r2_runner import parity, reference, cases
from tools.ml.physics.growbox_psychrometrics import saturation_vapor_density_kg_m3
from tools.ml.physics.growbox_tensor import simulate_batch

SCHEMA = 'bloomml-r4-dual-t4-stress-v1'
CATEGORIES = ('typical', 'wet_cold_surfaces', 'dry_airflow', 'night_cooling', 'lamp_heat')


def stress_cases(count: int, seed: int):
    """Heterogeneous synthetic chambers, forcing states, and fan curves."""
    parameters, states, forcings = cases(count, seed)
    rng = random.Random(seed ^ 0x52D4)
    categories = []
    for i, (p, s, f) in enumerate(zip(parameters, states, forcings)):
        category = CATEGORIES[i % len(CATEGORIES)]
        categories.append(category)
        if category == 'wet_cold_surfaces':
            states[i] = replace(s, wall_temperature_c=max(5,s.air_temperature_c-rng.uniform(5,12)),
                leaf_temperature_c=max(7,s.air_temperature_c-rng.uniform(3,7)),
                air_vapor_kg=saturation_vapor_density_kg_m3(s.air_temperature_c)
                    * p.geometry.air_volume_m3 * rng.uniform(.9,1.0))
            forcings[i] = replace(f, outside_temperature_c=rng.uniform(8,17),
                outside_relative_humidity_pct=rng.uniform(70,96),
                humidifier=rng.uniform(.15,.65), circulation_fan=rng.uniform(.4,1))
        elif category == 'dry_airflow':
            parameters[i] = replace(p, equipment=replace(p.equipment, exhaust_shutoff_pressure_pa=80.))
            forcings[i] = replace(f, outside_relative_humidity_pct=rng.uniform(10,30),
                exhaust_fan=rng.uniform(.55,.85),
                measured_pressure_difference_pa=-rng.uniform(0,25),
                measured_exhaust_m3_h=None, humidifier=0)
        elif category == 'night_cooling':
            forcings[i] = replace(f, lamp_dimmer=0, lamp_measured_electrical_w=None,
                cooler=rng.uniform(.1,.4), heater=0,
                outside_temperature_c=rng.uniform(7,15),
                outside_relative_humidity_pct=rng.uniform(30,90))
        elif category == 'lamp_heat':
            forcings[i] = replace(f, lamp_dimmer=rng.uniform(.6,1),
                lamp_measured_electrical_w=None, heater=0, cooler=0,
                outside_temperature_c=rng.uniform(17,25),
                exhaust_fan=rng.uniform(.25,.7))
    return parameters, states, forcings, categories


def physics_gate(parameters, final_states, water_residuals, *, dtype='float64'):
    """Reject physical bound failures, preserve distributions for plausibility."""
    if not final_states or len(parameters)!=len(final_states) or len(final_states)!=len(water_residuals):
        raise AssertionError('invalid physics batch lengths')
    temperature, humidity, carbon = [], [], []
    max_water, fog_count, surface_dew = 0., 0, 0
    for p, s, residual in zip(parameters,final_states,water_residuals):
        rh = (s.air_vapor_kg / p.geometry.air_volume_m3 /
              saturation_vapor_density_kg_m3(s.air_temperature_c) * 100.)
        if not (math.isfinite(rh) and -1e-6 <= rh <= 100.0001
                and 0 <= s.co2_ppm <= 100000
                and s.air_vapor_kg >= 0 and s.fog_condensate_kg >= -1e-8
                and s.wall_condensate_kg >= -1e-8 and s.leaf_condensate_kg >= -1e-8
                and s.cumulative_dli_mol_m2 >= 0):
            raise AssertionError(f'physical bounds: T={s.air_temperature_c} RH={rh} CO2={s.co2_ppm}')
        max_water = max(max_water,abs(residual))
        temperature.append(s.air_temperature_c)
        humidity.append(rh)
        carbon.append(s.co2_ppm)
        fog_count += s.fog_condensate_kg > 1e-9
        surface_dew += s.wall_condensate_kg > 1e-8 or s.leaf_condensate_kg > 1e-8
    if max_water > (3e-5 if dtype=='float32' else 1e-8):
        raise AssertionError('water conservation violation')
    return {
        'max_water_residual_kg':max_water,
        'air_temperature_c_min':min(temperature), 'air_temperature_c_max':max(temperature),
        'air_temperature_c_mean':sum(temperature)/len(temperature),
        'relative_humidity_pct_min':min(humidity), 'relative_humidity_pct_max':max(humidity),
        'relative_humidity_pct_mean':sum(humidity)/len(humidity),
        'co2_ppm_min':min(carbon), 'co2_ppm_max':max(carbon),
        'co2_ppm_mean':sum(carbon)/len(carbon),
        'scenarios_with_fog':fog_count,'scenarios_with_dew_on_surfaces':surface_dew,
    }


def run(*, source_dir: Path, output_dir: Path, seed=1729, scenarios_per_gpu=4096,
        ticks=180, dt_s=10., pilot_per_gpu=24):
    import torch
    if not torch.cuda.is_available() or torch.cuda.device_count()<2:
        raise RuntimeError('two independent CUDA GPUs required; no CPU fallback')
    if not 1 <= pilot_per_gpu <= scenarios_per_gpu or ticks <= 0 or not 0 < dt_s <= 3600:
        raise ValueError('invalid experiment dimensions')
    source_sha = subprocess.check_output(['git','-C',str(source_dir),'rev-parse','HEAD'],text=True).strip()
    if len(source_sha)!=40 or any(c not in '0123456789abcdef' for c in source_sha):
        raise RuntimeError('invalid source provenance')
    n = scenarios_per_gpu * 2
    parameters, states, forcings, categories = stress_cases(n,seed)

    def on_card(idx):
        a,b = idx*scenarios_per_gpu, (idx+1)*scenarios_per_gpu
        ps,ss,fs = (x[a:b] for x in (parameters,states,forcings))
        cpu = reference(ps[:pilot_per_gpu],ss[:pilot_per_gpu],fs[:pilot_per_gpu],
                        dt_s=dt_s,ticks=ticks)
        cuda_pilot = simulate_batch(ps[:pilot_per_gpu],ss[:pilot_per_gpu],fs[:pilot_per_gpu],
                                    dt_s=dt_s,ticks=ticks,device=f'cuda:{idx}',dtype=torch.float64)
        differences = parity(cpu,cuda_pilot.states,abs_tolerance=2e-6,relative_tolerance=2e-6)
        physics_gate(ps[:pilot_per_gpu],cuda_pilot.states,cuda_pilot.water_residuals_kg)
        torch.cuda.synchronize(idx)
        torch.cuda.reset_peak_memory_stats(idx)
        start = perf_counter()
        batch = simulate_batch(ps,ss,fs,dt_s=dt_s,ticks=ticks,
                               device=f'cuda:{idx}',dtype=torch.float64)
        torch.cuda.synchronize(idx)
        elapsed=perf_counter()-start
        return {
            'device':f'cuda:{idx}','gpu_name':torch.cuda.get_device_name(idx),
            'scenarios':scenarios_per_gpu,'ticks':ticks,
            'total_scenario_ticks':scenarios_per_gpu*ticks,
            'total_euler_substeps':sum(batch.substeps_per_tick)*ticks,
            'seconds':elapsed, 'scenario_ticks_per_second':scenarios_per_gpu*ticks/elapsed,
            'peak_cuda_torch_bytes':torch.cuda.max_memory_allocated(idx),
            'pilot_cpu_scalar_count':pilot_per_gpu,
            'pilot_cpu_cuda_max_absolute_error_by_field':differences,
            'bounds':physics_gate(ps,batch.states,batch.water_residuals_kg),
        }

    with ThreadPoolExecutor(max_workers=2) as executor:
        future0=executor.submit(on_card,0)
        future1=executor.submit(on_card,1)
        devices=[future0.result(),future1.result()]
    result={
        'schema':SCHEMA,'repository':'MichalMatu/bloomml','source_commit':source_sha,
        'execution':'kaggle','simulation_backend':'torch_cuda_batched_euler_r4',
        'synthetic_uncalibrated':True,'physical_validation_with_real_data':False,
        'meaning':'numerical_mass_energy_and_range_checks_only_not_measured_plant_prediction',
        'seed':seed,'scenarios_total':n,'ticks':ticks,'dt_s':dt_s,
        'simulated_minutes_per_scenario':ticks*dt_s/60.,
        'total_scenario_ticks':n*ticks,
        'cases_by_category':{c:categories.count(c) for c in CATEGORIES},
        'devices':devices,'torch_version':torch.__version__,
        'cuda_version':torch.version.cuda,'failed_physical_bounds':0,
    }
    output_dir.mkdir(parents=True,exist_ok=True)
    (output_dir/'r4_stress_manifest.json').write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    return result


def main():
    ap=argparse.ArgumentParser(description='Heavy dual-T4 BloomML numerical stress (not calibrated)')
    ap.add_argument('--source',type=Path,default=Path.cwd())
    ap.add_argument('--output',type=Path,default=Path('/kaggle/working/bloomml-r4'))
    ap.add_argument('--scenarios-per-gpu',type=int,default=4096)
    ap.add_argument('--ticks',type=int,default=180)
    ap.add_argument('--dt-s',type=float,default=10.)
    ap.add_argument('--pilot-per-gpu',type=int,default=24)
    ap.add_argument('--seed',type=int,default=1729)
    a=ap.parse_args()
    result=run(source_dir=a.source,output_dir=a.output,
        seed=a.seed,scenarios_per_gpu=a.scenarios_per_gpu,ticks=a.ticks,
        dt_s=a.dt_s,pilot_per_gpu=a.pilot_per_gpu)
    print(json.dumps({'source_commit':result['source_commit'],
        'scenario_ticks':result['total_scenario_ticks'],
        'devices':result['devices']},indent=2))


if __name__=='__main__':
    main()
