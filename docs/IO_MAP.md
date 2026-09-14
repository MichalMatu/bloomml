# I/O map

Compact production-oriented map. Full contract details live in schemas; research simulator inventory is separate.

## Production sources of truth

- production controller contract: `schemas/environment-controller.v6.json`;
- runtime architecture: `ARCHITECTURE.md`;
- current state: `CURRENT_STATUS.md`;
- work order: `PROJECT_ROADMAP.md`;
- RF identities: `RF433_DEVICE_CODES.md`.

The older `schemas/environment-controller.json` v4 contract and `docs/simulator/*` describe broad simulator/training tooling and are not the production climate-v6 runtime contract.

## Production measurement path

Current real-input composition uses semantic sources such as:

- inside SCD41 temperature / RH / CO2 with validity/freshness;
- configured BLE climate sources where enabled;
- DS3231/time/schedule state;
- runtime/output/storage diagnostics.

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

The operational RF reference currently covers:

- lamp;
- exhaust fan;
- humidifier.

Exact RF code/pulse/repeat evidence belongs in `RF433_DEVICE_CODES.md`. Shelly power-delta evidence belongs in `SHELLY_POWER_FEEDBACK.md`.

## Research/simulator I/O

The older v4 research contract models a broader growbox with up to four pots and 15 normalized outputs. That inventory remains useful to `tools/ml` research workflows but must not be used to infer production actuator ownership or current firmware behavior.

See `DATA_CONTRACT.md`, `CONFIG_MATRIX.md` and `simulator/IO_INVENTORY.md` only when working specifically on that research toolchain.

## Change rule

Do not add a production sensor/output by editing this summary first. Change the appropriate schema/config/domain boundary, generated artifacts and tests together, then update this map.
