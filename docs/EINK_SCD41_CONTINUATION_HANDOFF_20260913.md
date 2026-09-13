# E-ink + SCD41 continuation handoff

Updated: 2026-09-13
Repository: `MichalMatu/growbox-ml-controller`
Working branch: `feature/eink-clay-status`
Exact current branch HEAD: `01db8228e6d822b5c64359abd8bf2d85341b5919`
Local Agent binding: `815cf40f-8d2a-4e1f-b7cc-c0f4e37b6cb5`
Authorized board port: `/dev/cu.usbserial-1130`
Forbidden: `/dev/cu.usbserial-10`
Unauthorized without separate approval: `/dev/cu.usbserial-1120`

## What is already working

The CrowPanel 2.9-inch e-paper is physically operational on the authorized board.

Confirmed hardware profile:

- ESP32-S3-WROOM-1-N8R8 / 8 MB PSRAM
- display 128x296
- SSD1680Z backend
- SPI pins: SCLK 12, MOSI 11, CS 45, DC 46, RST 47, BUSY 48
- display power GPIO 7
- shared I2C remains SDA 21 / SCL 38

The display firmware was flashed successfully with physical outputs disabled. The board boots without reset loop, panic or watchdog evidence. E-ink physical refresh completes successfully.

Current display software milestone:

- orientation rotated to 180 degrees relative to the original visible orientation;
- large `Growbox Status` header removed and its layout area reclaimed;
- display is observer-side/read-only;
- e-ink work is off the control hot path;
- partial refresh uses SSD1680 partial mode (`0xFC`); full refresh is reserved for the required full-refresh cases;
- physical UART evidence showed a refresh completion with `kind=1 reason=4`, which is the project's `Partial + ContentChanged` path.

Host/display validation reached 5/5 display suites plus the full Stage27C CrowPanel build. The exact board candidate was flashed on `/dev/cu.usbserial-1130` with real physical outputs disabled.

## Current blocker: SCD41 shows no valid measurements

The physical display currently shows no sensor values and a warning icon in the corner.

UART evidence from the flashed candidate is stable for more than 130 seconds:

```text
scd_available=1
scd_sample=0
scd_read_errors=0
scd_invalid=0
scd_samples=0
```

At the same time other inputs are healthy:

```text
tp_sample=1
xiaomi_sample=1
rtc_available=1
rtc_trusted=1
sd_mounted=1
```

Therefore this is not a generic runtime crash, BLE failure, SD failure or display failure.

The display warning is explained by the current observer model: the display snapshot currently treats the SCD41 sample as the source of temperature, RH and CO2 validity. With `scd_sample=false`, all three climate values become invalid and the warning indicator is shown.

That display coupling should eventually be improved so the UI can show the best available authoritative readings, but **do not hide or weaken the SCD41 fault before diagnosing it**.

## Important history finding

The SCD41 driver itself is not the obvious regression point.

`Scd41InsideSource.cpp` functional history:

- `80601c83d` — initial native real-input skeleton
- `48435d6a6` — Sensirion native HAL declarations
- `de94a1fbf` — native Stage27 real input bundle
- `c8f231dbc` — I2C probe diagnostics
- `d0b52a352` — recover SCD41 after MCU-only reset
- `cf957a764` — Stage27C soak diagnostics
- `3ab26a98b` — 2026-09-12 grouping/refactor only

The important source behavior is still:

```cpp
const int16_t ready_error = scd4x_get_data_ready_status(&data_ready);
if (!data_ready) {
    return fillCached(monotonic_ms, output);
}
```

and `TelemetryReporter::record()` still calls:

```cpp
scd41_.sample(now_ms, scd_diag);
```

on every telemetry report.

The history audit did not identify a functional SCD41 driver change corresponding to the regression. I2C pins also did not change: both the old baseline and current CrowPanel profile use SDA 21 / SCL 38.

## What remains to determine

The next chat must establish why the SCD41 remains `available=true` but never reaches `hasMeasurement=true` / `scd_samples>0`.

The most useful evidence is the exact boot path and direct sensor behavior:

1. Confirm the actual boot log contains the SCD41 I2C probe and `begin()` result.
2. Confirm whether `scd4x_start_periodic_measurement()` succeeds or logs an error.
3. Confirm whether `scd4x_get_data_ready_status()` ever returns true.
4. Use the existing service-console `sensors` command to invoke the same `scd41_.sample()` path interactively and inspect the returned counters.
5. If needed, add a tiny bounded diagnostic to log the raw SCD4x return codes and ready-bit state; do not create a large monitoring subsystem.
6. Compare behavior against a known-good pre-regression executable or a minimal SCD41-only diagnostic build on the same board before changing the driver.

Do not conclude that the SCD41 hardware is faulty until this evidence exists.

## Reference project for the next UI stage

The operator has provided the local reference checkout:

`/Users/michal/Documents/PlatformIO/Projects/esp32s3_LiteGraph`

The intended next UI work is to reuse the proven Clay stack from that project instead of recreating it:

- Clay menu model/layout/navigation;
- Clay view/controller/page structure;
- physical button input/debounce/navigation;
- menu simulator/event model;
- glance/status/time modules where useful;
- ActionConfirmationDialog where bounded confirmation is needed;
- refresh policy and Clay view-mode management;
- host-side Clay tests/simulator infrastructure.

The growbox-specific state must remain a separate read-only snapshot/presenter layer. Do not copy unrelated LiteGraph application/settings/Wi-Fi ownership.

Important hard-binding rule: this conversation is permanently bound to `MichalMatu/growbox-ml-controller`. A Local Agent task from this conversation must not inspect or execute commands against the separate LiteGraph repository. The LiteGraph source must therefore be inspected only from a separately authorized repository context or from explicitly transferred source files/snippets.

## Frozen runtime/safety invariants

- deterministic Rule control remains authoritative;
- ML remains shadow/research-only;
- OutputSupervisor remains the only normal physical-output execution owner;
- UI must never execute RF433 directly;
- UI must never mutate safety state;
- UI must never fabricate physical actuator acknowledgement;
- real physical outputs remain disabled during unattended board qualification;
- display work must not block the 1-second controller loop.

Safety build/runtime fence values for unattended work:

```text
GROWBOX_RF433_LOOPBACK_ENABLED=0
GROWBOX_STAGE28_REAL_OUTPUTS_ENABLED=0
GROWBOX_STAGE28_THERMAL_TEST_SEQUENCE_ENABLED=0
```

## Do not redo completed work

Do not restart the entire e-ink backend implementation or re-litigate the panel pin map. The hardware/display stack is already operational and validated.

Do not change the SSD1680 partial command merely because the panel visibly changes during partial refresh. The current implementation already produces a logged `Partial + ContentChanged` refresh.

Do not port Clay before the SCD41/debug blocker is understood enough to avoid building the next UI on top of an unexplained sensor fault.

## Recommended next sequence

1. Diagnose SCD41 with exact boot evidence and service-console `sensors`.
2. Fix the smallest verified root cause and add a regression test.
3. Build, flash and verify SCD41 recovery on `/dev/cu.usbserial-1130` with physical outputs disabled.
4. Update the display snapshot policy so healthy authoritative non-SCD inputs can still be rendered without falsely masking the SCD41 fault.
5. Only then port the Clay/menu/button/simulator stack from the separate LiteGraph reference context.
6. Re-run host/simulator/ESP-IDF/display validation and update this handoff with the final exact SHA.

## Known task/tooling caveat

One recent Local Agent diagnostic task failed because `esptool.py` was not available in PATH (`exit 127`). That failure is tooling-only and is not evidence of a firmware fault. Prefer the repository's established Python/ESP-IDF environment (`scripts/source_idf.sh`, `scripts/stage27c_crowpanel.sh`, or the local venv) when running board commands.

Another first attempt at the `sensors` service-console task was rejected by the task-file JSON parser because of malformed escaping. No firmware change resulted. Re-run with a minimal valid task JSON and confirm the task result before drawing conclusions.
