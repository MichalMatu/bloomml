"""Deterministic CPU physics golden trajectories for later multi-GPU parity.

Synthetic prior ensembles are NOT physically calibrated and must never be
marked as true measurements or used to qualify autonomous ESP32 control.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import random
from dataclasses import asdict, replace
from pathlib import Path
from statistics import fmean

from .growbox_dynamics import (
    GrowboxEquipment, GrowboxForcing, GrowboxGeometry, GrowboxLighting,
    GrowboxPhysics, GrowboxState, GrowboxThermodynamics,
)


def sweep(*, scenarios: int, steps: int, seed: int, dt_s: float = 10.0,
          trajectories: Path | None = None) -> dict[str, object]:
    if not 1 <= scenarios <= 100_000 or not 1 <= steps <= 1_000_000 or not 0 < dt_s <= 60:
        raise ValueError("invalid workload parameters")
    rng = random.Random(seed)
    total_steps = 0
    water_max = 0.0
    end_air_t: list[float] = []
    end_rh: list[float] = []
    end_co2: list[float] = []
    end_speed: list[float] = []
    digest = hashlib.sha256()
    if trajectories is not None:
        trajectories.parent.mkdir(parents=True, exist_ok=True)
    with trajectories.open("w", encoding="utf-8") if trajectories is not None else _NullWriter() as output:
        for i in range(scenarios):
            volume = rng.uniform(0.3, 2.4)
            geometry = replace(
                GrowboxGeometry(),
                air_volume_m3=volume,
                wall_surface_area_m2=3.0 * (volume / .8) ** (2 / 3),
                canopy_area_m2=.7 * (volume / .8) ** (2 / 3),
                wall_heat_capacity_j_k=rng.uniform(70_000, 180_000) * (volume / .8) ** (2 / 3),
                wall_outside_conductance_w_k=rng.uniform(3, 15) * (volume / .8) ** (2 / 3),
            )
            lighting = replace(GrowboxLighting(), rated_power_w=rng.uniform(80, 250) * volume)
            equipment = replace(
                GrowboxEquipment(),
                exhaust_max_m3_h=rng.uniform(60, 145) * volume,
                infiltration_ach=rng.uniform(.05, .8),
                heater_power_w=rng.uniform(100, 260) * volume,
            )
            physics = GrowboxPhysics(geometry=geometry, lighting=lighting, equipment=equipment)
            inside_t = rng.uniform(18, 30)
            inside_rh = rng.uniform(35, 80)
            model = GrowboxThermodynamics(physics, GrowboxState.initial(
                air_temperature_c=inside_t, rh_pct=inside_rh, co2_ppm=rng.uniform(410, 1100),
                volume_m3=volume, substrate_water_kg=rng.uniform(.3, 2.4)))
            outside_t = rng.uniform(5, 36)
            outside_rh = rng.uniform(30, 90)
            fan, heater, cool, hum, dehum, co2, circulation = (rng.random() for _ in range(7))
            for step in range(steps):
                # Disturbance and lamp changes are deterministic and bounded.
                t = step * dt_s
                lamp = 1.0 if (t % 86_400) < 43_200 else 0.0
                forcing = GrowboxForcing(
                    outside_temperature_c=outside_t + 2 * math.sin(t / 3600),
                    outside_relative_humidity_pct=outside_rh,
                    outside_co2_ppm=420,
                    lamp_dimmer=lamp,
                    heater=heater, cooler=cool, exhaust_fan=fan, circulation_fan=circulation,
                    humidifier=hum, dehumidifier=dehum, co2_doser=co2,
                )
                model.step(forcing, dt_s)
                state = model.state
                obs = model.observe(forcing)
                if not 0 <= obs.rh_pct <= 100.01:
                    raise AssertionError("unphysical RH")
                residual = abs(model.water_balance_residual_kg())
                water_max = max(water_max, residual)
                if residual > 1e-8:
                    raise AssertionError("water mass conservation failed")
                row = {
                    "schema": "growbox-thermodynamic-golden-v2",
                    "origin": "synthetic_uncalibrated",
                    "seed": seed, "scenario": i, "step": step, "t_s": (step + 1) * dt_s,
                    "air_t_c": state.air_temperature_c,
                    "wall_t_c": state.wall_temperature_c,
                    "leaf_t_c": state.leaf_temperature_c,
                    "soil_t_c": state.substrate_temperature_c,
                    "rh_pct": obs.rh_pct, "co2_ppm": state.co2_ppm,
                    "vpd_air_kpa": obs.air_vpd_kpa,
                    "vpd_leaf_kpa": obs.leaf_vpd_kpa,
                    "dewpoint_c": obs.dew_point_c,
                    "ppfd_umol_m2_s": obs.ppfd_umol_m2_s,
                    "circulation_fan_fraction": circulation,
                    "estimated_or_measured_air_speed_m_s": obs.canopy_air_speed_m_s,
                    "air_speed_source": obs.canopy_air_speed_source,
                    "dli_mol_m2": obs.dli_mol_m2,
                    "soil_water_kg": state.substrate_water_kg,
                    "condensate_wall_kg": state.wall_condensate_kg,
                    "condensate_leaf_kg": state.leaf_condensate_kg,
                    "condensate_cooler_kg": state.cooler_condensate_kg,
                    "mass_residual_kg": model.water_balance_residual_kg(),
                }
                payload = json.dumps(row, sort_keys=True, separators=(",", ":"), allow_nan=False)
                digest.update(payload.encode("utf-8"))
                digest.update(b"\n")
                if trajectories is not None:
                    output.write(payload + "\n")
                total_steps += 1
            end_air_t.append(model.state.air_temperature_c)
            end_rh.append(model.observe(forcing).rh_pct)
            end_co2.append(model.state.co2_ppm)
            end_speed.append(model.observe(forcing).canopy_air_speed_m_s)
    return {
        "schema": "growbox-thermodynamic-golden-summary-v2",
        "physics_revision": "mixed-convection-v2",
        "origin": "synthetic_uncalibrated",
        "seed": seed, "scenarios": scenarios, "steps_per_scenario": steps,
        "time_step_s": dt_s, "total_steps": total_steps,
        "sha256": digest.hexdigest(),
        "max_water_residual_kg": water_max,
        "mean_final_air_temperature_c": fmean(end_air_t),
        "mean_final_rh_pct": fmean(end_rh),
        "mean_final_co2_ppm": fmean(end_co2),
        "mean_final_canopy_air_speed_m_s": fmean(end_speed),
        "ready_for_hardware": False, "ready_for_calibrated_training": False,
    }


class _NullWriter:
    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc, tb):
        return False


def main() -> int:
    parser = argparse.ArgumentParser(description="Versioned CPU physics trajectories for GPU numerical parity")
    parser.add_argument("--scenarios", type=int, default=32)
    parser.add_argument("--steps", type=int, default=360)
    parser.add_argument("--seed", type=int, default=1729)
    parser.add_argument("--dt-s", type=float, default=10.0)
    parser.add_argument("--output", type=Path, help="Optional NDJSON output; may be large")
    parser.add_argument("--summary", type=Path, help="Optional JSON summary")
    args = parser.parse_args()
    result = sweep(scenarios=args.scenarios, steps=args.steps, seed=args.seed,
                   dt_s=args.dt_s, trajectories=args.output)
    text = json.dumps(result, sort_keys=True, indent=2) + "\n"
    print(text, end="")
    if args.summary is not None:
        args.summary.parent.mkdir(parents=True, exist_ok=True)
        args.summary.write_text(text, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
