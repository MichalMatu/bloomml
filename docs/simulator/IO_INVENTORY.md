# v4 research I/O inventory

Scope: older `schemas/environment-controller.json` simulator/tooling contract only. This is not the production climate-v6 runtime contract.

Verify the research toolchain directly with:

```bash
python -c "from tools.ml import summarize_training_fields; print(summarize_training_fields())"
```

## Shape

| Item | Count |
| --- | ---: |
| Features | 128 |
| Outputs | 15 |
| Max pots | 4 |

## Output order

1. `heater`
2. `fan`
3. `humidifier`
4. `dehumidifier`
5. `cooler`
6. `co2_doser`
7. `irrigation_pot_1` ... `irrigation_pot_4`
8. `nutrient_heater`
9. `heat_mat_pot_1` ... `heat_mat_pot_4`

All are normalized to `[0, 1]`; research safety logic may force unavailable/unsafe outputs to zero.

## Feature groups

The 128 features cover:

- live growbox/outside/nutrient measurements plus validity masks;
- up to four pot availability/soil measurements/validity;
- `lights_active` context;
- growbox/environment configuration;
- actuator capability/availability;
- targets/setpoints;
- previous commands;
- per-pot cultivation, irrigation and heat-mat configuration/state.

Exact field order/ranges come from `schemas/environment-controller.json`, not this summary.

## Mix-and-match semantics

- sensor disabled/invalid -> validity mask false and contract-default imputation;
- actuator unavailable -> zero capability and safe-zero output;
- pot unavailable -> pot-specific targets/features canonicalized and pot outputs safe-zero.

## Boundary

PPFD, leaf temperature, EC/pH, flood sensors and additional weather/exhaust sensors are outside this v4 vector.

For current production work use `../IO_MAP.md`, `../ARCHITECTURE.md` and `../CURRENT_STATUS.md`. For this research toolchain use `../DATA_CONTRACT.md`, `../CONFIG_MATRIX.md` and `README.md`.
