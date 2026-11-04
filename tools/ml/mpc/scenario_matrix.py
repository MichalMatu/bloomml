"""Small, reproducible CPU fault/forcing envelope matrix; research-only offline evidence.

Does not qualify physical model, use external greenhouse measurements or command hardware.
The scenario matrix is intentionally diagnostic: many cases are *not* feasible.
"""
from __future__ import annotations

import argparse
from dataclasses import asdict, dataclass, replace
import hashlib
import json
from pathlib import Path
import random
import subprocess

from .contracts import MPCConfig, Targets
from .lab import run_episode


@dataclass(frozen=True)
class Scenario:
    name: str
    initial_t: float = 24.
    initial_rh: float = 60.
    initial_co2: float = 700.
    first_out_t: float = 22.
    second_out_t: float = 22.
    first_out_rh: float = 55.
    second_out_rh: float = 55.
    first_out_co2: float = 420.
    second_out_co2: float = 420.
    first_setpoint_t: float = 24.
    second_setpoint_t: float = 24.
    target_rh: float = 60.
    target_co2: float = 750.
    low_capacity: bool = False
    heater_failure: bool = False
    stale_sensor: bool = False


SCENARIOS = (
    Scenario('cold_start', initial_t=16, initial_rh=35, initial_co2=400),
    Scenario('hot_wet_start', initial_t=32, initial_rh=85, initial_co2=1500),
    Scenario('cold_front', first_out_t=22, second_out_t=8, second_out_rh=80),
    Scenario('heat_wave', first_out_t=22, second_out_t=38, second_out_rh=25),
    Scenario('humidity_swing', first_out_rh=95, second_out_rh=10),
    Scenario('co2_environment_shift', first_out_co2=420, second_out_co2=1250),
    Scenario('target_change', first_setpoint_t=22, second_setpoint_t=28,
             target_co2=950, target_rh=65),
    Scenario('heater_failed', initial_t=17, heater_failure=True),
    Scenario('stale_sensor', stale_sensor=True),
    Scenario('insufficient_capacity', initial_t=16, low_capacity=True),
    Scenario('competing_targets', initial_t=31, initial_rh=80, initial_co2=400,
             second_out_t=37, second_out_rh=95, target_co2=1600),
)


def source_sha() -> str | None:
    try:
        value = subprocess.check_output(['git', 'rev-parse', 'HEAD'],
                                         text=True, stderr=subprocess.DEVNULL).strip()
        return value if len(value) == 40 and all(x in '0123456789abcdef' for x in value) else None
    except (FileNotFoundError, subprocess.CalledProcessError):
        return None


def run_matrix(*, steps: int, step_s: float, seeds: tuple[int, ...], output: Path) -> dict:
    """Never edits production or mutates a shared controller; one fresh plant per policy."""
    from tools.ml.physics.growbox_dynamics import (
        GrowboxForcing, GrowboxPhysics, GrowboxState,
    )
    if not 3 <= steps <= 288 or not 5 <= step_s <= 600 or not seeds:
        raise ValueError('invalid diagnostic matrix bounds')
    sha = source_sha()
    output.mkdir(parents=True, exist_ok=True)
    manifest = {'source_commit':sha or 'UNKNOWN', 'synthetic_uncalibrated':True,
                'physical_validation_with_real_data':False,
                'config': {'steps': steps, 'step_s': step_s, 'seeds': list(seeds)},
                'scenarios': [asdict(s) for s in SCENARIOS],
                'results':[], 'failed_executions':[]}
    for scenario in SCENARIOS:
        for seed in seeds:
            # Changes are deterministic and identical between all controllers.
            rng = random.Random(seed + sum(ord(c) for c in scenario.name))
            physics = GrowboxPhysics()
            if scenario.low_capacity:
                physics = replace(physics, equipment=replace(physics.equipment,
                    heater_power_w=15, cooler_capacity_w=20, humidifier_output_g_h=8,
                    dehumidifier_removal_g_h=6, co2_injection_g_h=2,
                    exhaust_max_m3_h=9))
            initial = GrowboxState.initial(volume_m3=physics.geometry.air_volume_m3,
                air_temperature_c=scenario.initial_t + rng.uniform(-.25,.25),
                rh_pct=max(1, min(99,scenario.initial_rh + rng.uniform(-1,1))),
                co2_ppm=max(100,scenario.initial_co2 + rng.uniform(-15,15)))
            mid = (steps // 2) * step_s
            forcing = lambda t: GrowboxForcing(
                outside_temperature_c=scenario.first_out_t if t < mid else scenario.second_out_t,
                outside_relative_humidity_pct=scenario.first_out_rh if t < mid else scenario.second_out_rh,
                outside_co2_ppm=scenario.first_out_co2 if t < mid else scenario.second_out_co2)
            targets = lambda t: Targets(
                temperature_c=scenario.first_setpoint_t if t < mid else scenario.second_setpoint_t,
                rh_pct=scenario.target_rh, co2_ppm=scenario.target_co2,
                ppfd_umol_m2_s=250.)
            faults = (lambda t: frozenset({'heater'}) if t >= mid else frozenset()) \
                if scenario.heater_failure else None
            missing = (lambda t: 500. if t >= mid else 0.) \
                if scenario.stale_sensor else None
            cfg = MPCConfig(step_s=step_s, horizon_steps=2, beam_width=2,
                            max_evaluations=28, time_limit_s=20., seed=seed)
            for policy in ('mpc','threshold','hysteresis'):
                key = f'{scenario.name}_seed{seed}_{policy}'
                try:
                    metrics, records = run_episode(physics, initial, forcing, targets,
                        steps=steps, cfg=cfg, policy=policy,
                        faulted_at=faults, missing_at=missing,
                        source_commit=sha)
                except Exception as exc:
                    # Inability to finish is a failure, never a skipped PASS.
                    manifest['failed_executions'].append({'key':key,'exception':type(exc).__name__,
                                                         'message':str(exc)[:300]})
                    continue
                lines = [json.dumps(r,sort_keys=True,allow_nan=False,separators=(',',':'))
                         for r in records]
                (output/f'{key}.ndjson').write_text('\n'.join(lines)+'\n',encoding='utf-8')
                manifest['results'].append({'scenario':scenario.name,'seed':seed,'policy':policy,
                    'quality':asdict(metrics),
                    'trajectory_sha256': hashlib.sha256(('\n'.join(lines)+'\n').encode()).hexdigest()})
    states = [r['quality']['status'] for r in manifest['results']]
    manifest['quality_gate'] = ('FAIL' if manifest['failed_executions'] or 'FAIL' in states
                              else 'BLOCKED' if 'BLOCKED' in states
                              else 'UNKNOWN' if not sha or 'UNKNOWN' in states
                              else 'PASS')
    (output/'manifest.json').write_text(json.dumps(manifest,sort_keys=True,indent=2,
                                                allow_nan=False)+'\n',encoding='utf-8')
    return manifest


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--steps',type=int,default=12)
    parser.add_argument('--step-s',type=float,default=60.)
    parser.add_argument('--seeds',type=int,nargs='+',default=[1729])
    parser.add_argument('--output-dir',type=Path,required=True)
    args=parser.parse_args(argv)
    report=run_matrix(steps=args.steps,step_s=args.step_s,seeds=tuple(args.seeds),output=args.output_dir)
    print(json.dumps({'quality_gate':report['quality_gate'],'source_commit':report['source_commit'],
           'runs':len(report['results']),'failed_executions':report['failed_executions'],
           'failures':[{'scenario':r['scenario'],'seed':r['seed'],'policy':r['policy'],
                        'status':r['quality']['status']} for r in report['results']
                       if r['quality']['status']!='PASS']},sort_keys=True))
    return 0 if report['quality_gate']=='PASS' else 2


if __name__=='__main__':
    raise SystemExit(main())
