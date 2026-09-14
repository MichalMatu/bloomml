# Config matrix — v4 research tooling

Status: research/tooling reference for the older `schemas/environment-controller.json` v4 contract. It is **not** the production climate-v6 runtime contract.

Machine source of truth: `docs/CONFIG_MATRIX.csv`.
Current matrix size: 59 discrete profiles.

## Purpose

The matrix exercises mix-and-match availability/validity/configuration combinations for the broad v4 growbox model used by parts of `tools/ml`.

It covers combinations such as:

- sensor validity;
- global actuator availability;
- 0-4 pot availability;
- irrigation/heat-mat availability and control type;
- representative single-fault cases;
- bounded continuous min/default/max sweeps.

It deliberately does not enumerate every continuous combination. The CSV is the canonical profile list; do not duplicate all 59 rows in Markdown.

## Main host command

```bash
python -m tools.ml.run_config_matrix
```

Default input/output:

```text
docs/CONFIG_MATRIX.csv
-> build/audit/CONFIG_MATRIX_RESULTS.csv
```

Optional replay scripts:

```bash
python -m tools.ml.config_matrix --write-scripts build/matrix-replay
```

## Board research audits

These are historical/research test surfaces and require an explicitly authorized board port:

```bash
python -m tools.ml.board_engine_audit --matrix-only --report build/audit/board_config_matrix.json
python -m tools.ml.validity_matrix_audit --port <authorized-port>
```

Do not copy old example serial paths from Git history. Current hardware authorization comes from `../AGENTS.md` and `CURRENT_STATUS.md`.

## Required assertions

For each applicable profile/tool path verify:

- encoded values are finite and within the contract range;
- model/schema dimensions and hash match the v4 toolchain;
- unavailable actuators resolve to safe zero;
- invalid sensor masks use the contract default/imputation semantics;
- unavailable pots force pot outputs safe-zero;
- simulator steps remain finite and within bounded contract ranges.

## Production boundary

Production controller work uses `schemas/environment-controller.v6.json` and the architecture in `ARCHITECTURE.md`.

Do not use this matrix to infer production output ownership, the active firmware I/O surface, or the current display roadmap. See `DATA_CONTRACT.md` for the contract split.
