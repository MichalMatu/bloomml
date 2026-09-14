# Config matrix — v4 research tooling

Status: research/tooling reference for the older `schemas/environment-controller.json` v4 contract. It is **not** the production climate-v6 runtime contract.

Machine source of truth: `docs/CONFIG_MATRIX.csv`.
Current matrix size: 59 discrete profiles.

## Purpose

The matrix exercises mix-and-match availability/validity/configuration combinations for the broad v4 growbox model used by parts of `tools/ml`.

It covers sensor validity, global actuator availability, 0-4 pot availability, irrigation/heat-mat control type, representative single-fault cases and bounded continuous min/default/max sweeps.

The CSV owns profile contents. The compact ID list below is retained because CI checks that this human index and CSV contain the same profile set.

## Profile IDs

| ID |
| --- |
| `P01` |
| `P02` |
| `P03` |
| `P04` |
| `P05` |
| `P06` |
| `P07` |
| `P08` |
| `P09` |
| `P10` |
| `P11` |
| `P12` |
| `P13` |
| `P14` |
| `P15` |
| `P16` |
| `P17` |
| `P18` |
| `P19` |
| `P20` |
| `S_OFF_air_temperature_c` |
| `S_OFF_air_humidity_pct` |
| `S_OFF_co2_ppm` |
| `S_OFF_nutrient_solution_temperature_c` |
| `S_OFF_outside_temperature_c` |
| `S_OFF_outside_humidity_pct` |
| `S_OFF_outside_co2_ppm` |
| `A_OFF_heater` |
| `A_OFF_fan` |
| `A_OFF_humidifier` |
| `A_OFF_dehumidifier` |
| `A_OFF_cooler` |
| `A_OFF_co2_doser` |
| `A_OFF_nutrient_heater` |
| `POT1_0001` |
| `POT1_0010` |
| `POT1_0011` |
| `POT1_0100` |
| `POT1_0101` |
| `POT1_0110` |
| `POT1_0111` |
| `POT1_1000` |
| `POT1_1001` |
| `POT1_1010` |
| `POT1_1011` |
| `POT1_1100` |
| `POT1_1101` |
| `POT1_1110` |
| `POT1_1111` |
| `CTRL_binary_binary` |
| `CTRL_binary_pwm` |
| `CTRL_pwm_binary` |
| `CTRL_pwm_pwm` |
| `ONLY_POT_1` |
| `ONLY_POT_2` |
| `ONLY_POT_3` |
| `ONLY_POT_4` |
| `L0` |
| `L1` |

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

These are research test surfaces and require an explicitly authorized board port:

```bash
python -m tools.ml.board_engine_audit --matrix-only --report build/audit/board_config_matrix.json
python -m tools.ml.validity_matrix_audit --port <authorized-port>
```

Current hardware authorization comes from `../AGENTS.md` and `CURRENT_STATUS.md`.

## Required assertions

For each applicable profile/tool path verify finite/in-range encoded values, matching v4 dimensions/hash, safe-zero for unavailable outputs, validity/imputation behavior, unavailable-pot zeroing and bounded simulator state.

## Production boundary

Production controller work uses `schemas/environment-controller.v6.json` and `ARCHITECTURE.md`.

Do not use this matrix to infer production output ownership, active firmware I/O or the display roadmap. See `DATA_CONTRACT.md` for the contract split.
