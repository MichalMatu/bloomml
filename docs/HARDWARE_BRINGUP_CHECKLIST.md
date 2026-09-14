# Hardware Bring-up Checklist

Status: historical bring-up reference. Current debugging state is tracked in `docs/CURRENT_STATUS.md`.

Passing software checks is software evidence only; it is not hardware validation.

## Frozen software boundary before hardware work

The controller architecture is ready for concrete hardware adapters without changing the climate core:

- `CompositeClimateSnapshotProvider` aggregates hardware-neutral inside, outside, clock and schedule/config sources into the existing `ClimateInputSnapshot`;
- `ClimateApplication` keeps the strict input -> processing -> output composition;
- `MappedClimateRoleDriver` maps six stable semantic actuator roles to normalized endpoint writes;
- `ClimateControlLoop` remains the owner of confirmed applied state, OFF recovery and actuator fault latching;
- diagnostics observe the exact consumed input and resulting runtime/output evidence without feeding control state;
- Rule remains authoritative. `MlActive` is not qualified for real actuation.

## Frozen framework/platform policy

Hardware implementation remains **100% native ESP-IDF v5.5.4**.

- Do not add Arduino-ESP32 as an ESP-IDF component.
- Do not move the project to PlatformIO/Arduino.
- Arduino/PlatformIO projects may be reference/donor sources only.
- Reuse framework-independent protocol decoders, register definitions, constants, pin maps and tested behavior when useful, but keep board/runtime ownership native ESP-IDF.

Preferred first platform is the Elecrow CrowPanel 2.9-inch e-paper ESP32-S3 already used by the user's `esp32s3_LiteGraph` hardware and custom HAT.

Fallback remains a plain inexpensive ESP32-S3 devboard if CrowPanel-specific support becomes disproportionate engineering risk.

## Real-input choices

- Inside sensor: Sensirion SCD41, native ESP-IDF only; failed reads remain unavailable/invalid, never synthetic zero values.
- Outside BLE sensor: native ESP-IDF NimBLE; configured sensor selected deterministically by stable identity; malformed/foreign/partial advertisements must not refresh climate freshness.
- RTC: DS3231 with backup battery, native ESP-IDF I2C only; lost/untrusted time remains invalid until intentionally synchronized.
- I2C: one explicit native ESP-IDF bus owner for SCD41 + DS3231 when wiring allows it.

## Reference/donor repositories

Use as evidence/reference, not runtime dependencies:

- `MichalMatu/esp32s3_LiteGraph` for proven CrowPanel/HAT wiring and display/button behavior;
- `MichalMatu/MatrixHub` for selected BLE/protocol references.

## Outputs during input debugging

Physical outputs stay fake/locked while validating SCD41/BLE/DS3231 unless a separate bounded output qualification explicitly authorizes otherwise.

- Do not energize growbox loads during sensor debugging.
- Verify availability, measurement validity, age/freshness and schedule transitions through diagnostics.
- Keep Rule authoritative and ML shadow/research only.

## Evidence required during physical bring-up

For every real input/output backend, retain evidence for:

- exact source revision and hardware configuration;
- detected device/part identity where available;
- bounded startup and diagnostics logs;
- normal readings/writes plus stale/unavailable/rejection cases;
- trusted/untrusted RTC behavior;
- confirmed safe OFF behavior before physical output work;
- no panic/reset loop during the bounded validation window;
- Rule-authoritative operation before any ML shadow collection.

A successful ESP-IDF build, host test or fake-runtime run is not a substitute for these physical checks.

For the current workflow, repository branches and hardware boundaries use `AGENTS.md`, `docs/CURRENT_STATUS.md` and `docs/PROJECT_ROADMAP.md`. Historical Stage27/28 details remain available in Git history and are summarized in `docs/HISTORY.md`.
