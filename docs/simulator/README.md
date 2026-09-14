# Growbox simulator research

Status: active research/tooling area for the older broad v4 growbox contract, not the production climate-v6 runtime contract.

Source contract for this directory: `schemas/environment-controller.json` (v4, 128 features, 15 outputs, up to four pots).

Production firmware/controller ownership is documented in `../ARCHITECTURE.md` and uses `schemas/environment-controller.v6.json`. Do not mix the two contracts.

## What this simulator is for

- lumped growbox air and pot-substrate physics research;
- deterministic synthetic trajectories;
- open-loop calibration tooling;
- deviations/foresight experiments;
- optional PyVista scientific twin visualization;
- legacy/broad v4 dataset/model experiments.

It is not physical qualification and does not override production Rule authority.

## Main entry points

```text
tools/ml/simulator.py
tools/ml/physics/
tools/ml/calibration.py
tools/ml/calibrate_simulator.py
tools/ml/deviations.py
tools/ml/foresight.py
tools/ml/twin/
```

Useful commands:

```bash
python -m tools.ml.probe_simulator
python -m tools.ml.calibrate_simulator protocol
python -m tools.ml.calibrate_simulator demo --out-dir build/calibration-demo
python -m tools.ml.twin_view --live
```

## Current research direction

The next useful simulator step is calibration against real growbox trajectories, not another broad synthetic training sweep. Fit a small number of physically interpretable coefficients, re-run directional/closed-loop checks, then reconsider training only if the calibrated data creates a concrete hypothesis.

## Documents in this directory

| Document | Purpose |
| --- | --- |
| `CALIBRATION.md` | real-box -> lumped-parameter fitting protocol |
| `FORMULAS.md` | physics equations/parameter notes |
| `PHYSICS_SCOPE.md` | allowed simulator scope |
| `VALIDATION.md` | directional/behavior validation |
| `IO_INVENTORY.md` | v4 128-feature / 15-output inventory |
| `SLOT_MAP.md` | external model symbols -> v4 slots |
| `SOURCES.md` | research sources/licenses |
| `DEPENDENCIES.md` | research dependency/catalog notes |
| `TWIN_VIEW.md` | PyVista visualization |

For production/current work start from `../README.md` and `../CURRENT_STATUS.md`, not from this directory.
