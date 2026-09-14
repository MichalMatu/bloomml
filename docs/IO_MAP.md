# I/O map

Compact production-oriented map. Full contract details live in schemas; research simulator inventory is separate.

## Production sources of truth

- production controller contract: `schemas/environment-controller.v6.json`;
- runtime architecture: `ARCHITECTURE.md`;
- current state: `CURRENT_STATUS.md`;
- work order: `PROJECT_ROADMAP.md`;
- RF identities: `RF433_DEVICE_CODES.md`.

The older `schemas/environment-controller.json` v4 contract and `docs/simulator/*` describe broad simulator/training tooling and are not the production climate-v6 runtime contract.

## Current real-input composition

The production real-input adapter currently composes inside/nearby state as follows:

| Semantic role | Current source | Production use |
| --- | --- | --- |
| inside temperature | TP357 BLE | inside air temperature |
| inside relative humidity | TP357 BLE | inside RH |
| inside CO₂ | SCD41 | CO₂ only in the composed inside snapshot |
| nearby/outside temperature | Xiaomi BLE | nearby/outside air temperature |
| nearby/outside relative humidity | Xiaomi BLE | nearby/outside RH |
| wall clock | DS3231/runtime clock path | schedule/time context |

The SCD41 driver may expose temperature/RH internally, but current `RuntimeInsideSource` intentionally takes T/RH from TP357 and only copies valid SCD41 CO₂ into the production inside snapshot.

Missing or stale hardware remains unavailable/invalid; it is never represented as a fake successful zero measurement.

## Production output path

```text
measurements + schedule/config
            |
            v
    ClimateApplication
            |
            v
 deterministic Rule intent
            |
            v
 safety / binary policy
            |
            v
    OutputSupervisor
            |
            v
 RuntimeOutputTransport
            |
            v
        RF433
```

Requested, resolved, transport-attempted/executed and independently observed physical state remain distinct.

The e-ink/UI path is observer-only and cannot become an output owner.

## Current configured growbox loads

The operational RF reference covers:

- lamp;
- exhaust fan;
- humidifier.

Exact RF code/pulse/repeat evidence belongs in `RF433_DEVICE_CODES.md`. Shelly power-delta evidence belongs in `SHELLY_POWER_FEEDBACK.md`.

## Research/simulator I/O

The older v4 research contract models a broader growbox with up to four pots and 15 normalized outputs. It remains useful to `tools/ml` research workflows but must not be used to infer production actuator ownership or current firmware behavior.

See `DATA_CONTRACT.md`, `CONFIG_MATRIX.md` and `simulator/IO_INVENTORY.md` only for that research toolchain.

## Change rule

Do not add a production sensor/output by editing this summary first. Change the appropriate schema/config/domain boundary, generated artifacts and tests together, then update this map.
