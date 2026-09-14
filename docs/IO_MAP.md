# I/O map

This file is a compact operator/developer map. It intentionally does **not** duplicate the full schema, historical v1/v2 plans, simulator equations, or old product-roadmap discussion.

## Sources of truth

- Contract and field definitions: [`schemas/environment-controller.json`](../schemas/environment-controller.json)
- Contract rules and regeneration: [DATA_CONTRACT.md](DATA_CONTRACT.md)
- Full ML feature/output inventory: [simulator/IO_INVENTORY.md](simulator/IO_INVENTORY.md)
- Current firmware/runtime state: [CURRENT_STATUS.md](CURRENT_STATUS.md)
- Current product order of work: [PROJECT_ROADMAP.md](PROJECT_ROADMAP.md)
- Simulator/physics details: [simulator/README.md](simulator/README.md)

If this summary disagrees with the schema or generated contract, the schema wins.

## Physical model

One controller represents one growbox air volume with up to four pots.

- Air measurements are shared by the growbox.
- Pot soil measurements and pot actuators are independent per pot.
- Every measurement has explicit validity/freshness semantics; missing hardware is never represented as a fake zero measurement.
- Every actuator has explicit availability/capability semantics.
- Hardware adapters remain outside the portable controller/model contract.

## Main measurement groups

| Group | Measurements |
|---|---|
| Growbox air | temperature, relative humidity, CO2 |
| Intake/outside air | temperature, relative humidity, optional CO2 |
| Nutrient solution | solution temperature |
| Pots 1-4 | soil moisture and soil temperature per pot |
| System context | time/schedule and light-active state where configured |

`lights_active` is system/schedule context, not a physical environmental sensor.

## Main actuator groups

The schema/model surface contains normalized actuator commands. Availability and safety may force an unavailable or unsafe output to zero.

| Group | Outputs |
|---|---|
| Climate | heater, fan, humidifier, dehumidifier, cooler, CO2 dosing |
| Nutrient solution | nutrient heater |
| Pots 1-4 | irrigation and heat-mat command per pot |

Lighting schedule/output ownership is intentionally separate from the ML output vector unless the contract is explicitly revised.

## Runtime boundary

The current ESP32-S3 production-oriented runtime keeps these concerns separate:

```text
hardware measurements/config/time
            |
            v
      semantic snapshot
            |
            v
   controller / policy
            |
            v
 deterministic safety
            |
            v
    OutputSupervisor
            |
            v
    physical transport
```

The e-ink/UI path is observer-only and must not become an output owner.

## Current hardware-debugging focus

The active debugging blocker is SCD41 sampling on the CrowPanel build. During this work:

- SCD41 provides inside temperature/RH/CO2 when valid;
- outside BLE and DS3231 continue as independent input sources;
- physical outputs remain disabled/fake-locked;
- no I/O-contract expansion is part of the SCD41 fix.

See [CURRENT_STATUS.md](CURRENT_STATUS.md) for the exact evidence and debugging sequence.

## Change rule

Do not add a sensor/output by editing this file first. A contract change requires the schema and generated artifacts/tests to change together, followed by any required model regeneration/retraining and runtime adaptation.
