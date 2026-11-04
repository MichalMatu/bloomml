"""Reproducible offline MPC/threshold/hysteresis comparison on identical disturbances."""
from __future__ import annotations
import argparse
import json
from dataclasses import asdict
from pathlib import Path
import subprocess
from .contracts import MPCConfig, Targets
from .lab import run_episode


def _source_commit():
    try:
        return subprocess.check_output(['git', 'rev-parse', 'HEAD'], stderr=subprocess.DEVNULL,
                                       text=True).strip()
    except (FileNotFoundError, subprocess.CalledProcessError):
        return None


def main(argv=None) -> int:
    from tools.ml.physics.growbox_dynamics import GrowboxForcing, GrowboxPhysics, GrowboxState
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--hours', type=float, default=.5)
    parser.add_argument('--step-s', type=float, default=60)
    parser.add_argument('--seed', type=int, default=1729)
    parser.add_argument('--output-dir', type=Path, required=True)
    args = parser.parse_args(argv)
    if not 0 < args.hours <= 72 or not 0 < args.step_s <= 600:
        parser.error('research sweep supports 0<hours<=72, 0<step<=600s')
    steps = max(1, round(args.hours * 3600 / args.step_s))
    physics = GrowboxPhysics()
    initial = GrowboxState.initial(volume_m3=physics.geometry.air_volume_m3,
                                    air_temperature_c=20., rh_pct=55., co2_ppm=450.)
    def forcing(seconds):
        return GrowboxForcing(outside_temperature_c=10 if seconds >= args.hours * 1800 else 19,
                              outside_relative_humidity_pct=80 if seconds >= args.hours * 1800 else 50)
    def targets(seconds):
        return Targets(temperature_c=25 if seconds >= args.hours * 2400 else 23,
                       rh_pct=60, co2_ppm=800, ppfd_umol_m2_s=250)
    cfg = MPCConfig(step_s=args.step_s, seed=args.seed)
    sha = _source_commit()
    reports = {}
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for policy in ('mpc', 'threshold', 'hysteresis'):
        report, trajectory = run_episode(physics, initial, forcing, targets,
                  steps=steps, cfg=cfg, policy=policy, source_commit=sha)
        reports[policy] = asdict(report)
        with (args.output_dir / f'{policy}.ndjson').open('w', encoding='utf-8') as dest:
            for row in trajectory: dest.write(json.dumps(row, sort_keys=True) + '\n')
    manifest = {'source_commit': sha or 'UNKNOWN', 'seed': args.seed,
                'simulation_only': True, 'synthetic_uncalibrated': True,
                'physical_validation_with_real_data': False,
                'units': {'temperature':'C', 'RH':'percentage_points','CO2':'ppm',
                          'PPFD':'umol/m2/s', 'energy':'Wh_nameplate_proxy'},
                'config': {'step_s':args.step_s, 'steps':steps, 'hours':args.hours},
                'baselines': reports}
    (args.output_dir / 'manifest.json').write_text(json.dumps(manifest, indent=2, sort_keys=True)+'\n')
    print(json.dumps({'source_commit': sha or 'UNKNOWN', 'metrics': reports}, sort_keys=True))
    return 0 if all(r['status']=='PASS' for r in reports.values()) else 2

if __name__ == '__main__':
    raise SystemExit(main())
