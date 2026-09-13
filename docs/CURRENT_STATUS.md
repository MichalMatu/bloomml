# Current controller status

Updated: 2026-09-13
Repository: `MichalMatu/growbox-ml-controller`
Primary development branch: `main`
Active implementation branch: `feature/eink-clay-status`
Control branch: `agent-control`
Fresh-chat entrypoint: `docs/FRESH_CHAT_BOOTSTRAP.md`
Product roadmap: `docs/PROJECT_ROADMAP.md`
E-ink implementation handoff: `docs/EINK_UI_HANDOFF.md`
SCD41 continuation handoff: `docs/EINK_SCD41_CONTINUATION_HANDOFF_20260913.md`
SCD41 continuation prompt: `docs/EINK_SCD41_NEW_CHAT_PROMPT_20260913.md`

## Current phase

The operator-facing CrowPanel e-ink milestone is physically working and should not be reimplemented. The active blocker is now **SCD41 measurement regression/debugging**. After that blocker is closed, continue the Clay/menu/button/simulator port from the proven LiteGraph reference.

## Current exact firmware/branch state

Working branch HEAD at the time of the SCD41 handoff:

`01db8228e6d822b5c64359abd8bf2d85341b5919`

This candidate was built and flashed to the authorized CrowPanel board with physical outputs disabled. The board boots and the e-ink physically refreshes.

Authorized serial device:

`/dev/cu.usbserial-1130`

Never use `/dev/cu.usbserial-10`.

Do not use `/dev/cu.usbserial-1120` without separate authorization.

## E-ink status

The CrowPanel 2.9-inch DIE01129S001 display is operational.

Profile:

- ESP32-S3-WROOM-1-N8R8 / 8 MB PSRAM;
- 128x296 e-paper;
- SSD1680Z transport;
- SCLK 12, MOSI 11, CS 45, DC 46, RST 47, BUSY 48;
- display power GPIO 7;
- shared I2C SDA 21 / SCL 38.

Completed display work:

- 180-degree rotation;
- removed the large `Growbox Status` header and reclaimed its layout area;
- observer/read-only display ownership;
- asynchronous display worker kept outside the controller hot path;
- SSD1680 partial refresh path remains `0xFC`;
- physical refresh evidence includes `Partial + ContentChanged` (`kind=1 reason=4`).

Display validation reached all five display host suites and a full Stage27C CrowPanel build. The exact candidate was flashed with real physical outputs disabled.

## Active SCD41 blocker

The display currently shows no environment values and a warning `!` because the display snapshot treats the SCD41 sample as the validity source for temperature/RH/CO2.

Stable UART evidence after more than 130 seconds:

```text
scd_available=1
scd_sample=0
scd_read_errors=0
scd_invalid=0
scd_samples=0

tp_sample=1
xiaomi_sample=1
rtc_available=1
rtc_trusted=1
SD mounted/logging healthy
```

The SCD41 is therefore considered available, but no successful measurement has been recorded. This is the current debugging target.

History review did not identify an obvious functional SCD41 driver regression:

- `Scd41InsideSource.cpp` functional implementation predates the e-ink work;
- `3ab26a98b` on 2026-09-12 grouped/moved SCD41 input components rather than changing the measurement algorithm;
- `TelemetryReporter::record()` still calls `scd41_.sample()` on every telemetry report;
- I2C remains SDA 21 / SCL 38.

Do not hide the SCD41 fault before understanding it. Once the root cause is known, the display may be improved to show the best available authoritative non-SCD inputs without fabricating CO2 or masking a real sensor fault.

## Required next debugging steps

1. Run the existing service-console `sensors` command on the already flashed board.
2. Capture complete boot evidence for I2C probe, SCD41 `begin()`, periodic-measurement start and early telemetry.
3. If samples stay zero, log only the raw return codes/ready state for `scd4x_start_periodic_measurement`, `scd4x_get_data_ready_status` and `scd4x_read_measurement` when reached.
4. Compare with a known-good pre-regression executable or minimal SCD41-only diagnostic on the same board when useful.
5. Identify the exact root/regression commit before making a behavioral change.
6. Add the smallest fix plus regression test.
7. Build/flash/verify on `/dev/cu.usbserial-1130` with all physical-output fences disabled.

## Next UI stage after SCD41

Reference project supplied by the operator:

`/Users/michal/Documents/PlatformIO/Projects/esp32s3_LiteGraph`

Reuse proven components rather than recreating them:

- Clay menu/model/layout/navigation;
- Clay view/controller/page structure;
- physical button debounce/navigation;
- glance/status/time modules where useful;
- ActionConfirmationDialog where useful;
- refresh policy / Clay view-mode manager;
- Clay menu/display simulator and event model;
- host-side Clay tests.

The current chat is hard-bound to `growbox-ml-controller`; LiteGraph must not be inspected or executed from a Local Agent task created under this binding. Use a separately authorized repo context or explicitly transferred source files/snippets for that analysis.

## Frozen safety/output invariants

- deterministic Rule control remains authoritative;
- ML remains shadow/research-only;
- OutputSupervisor is the normal physical-output execution owner;
- UI never executes RF433 directly;
- UI never mutates safety state;
- UI never fabricates physical actuator acknowledgement;
- physical outputs remain disabled during unattended qualification;
- display work must not block the 1-second control loop.

Unattended safety fence values:

```text
GROWBOX_RF433_LOOPBACK_ENABLED=0
GROWBOX_STAGE28_REAL_OUTPUTS_ENABLED=0
GROWBOX_STAGE28_THERMAL_TEST_SEQUENCE_ENABLED=0
```

## Do not redo completed work

Do not reopen the CrowPanel pin map, SSD1680 backend, display initialization, async worker, 180-degree rotation, title removal or proven partial-refresh command unless new evidence directly requires it.

## Repository workflow

Every Local Agent task created by the bound chat must contain exactly:

```json
{
  "agent_binding": "815cf40f-8d2a-4e1f-b7cc-c0f4e37b6cb5",
  "work_branch": "feature/eink-clay-status",
  "resources": []
}
```

Use direct GitHub for small bounded source/config/docs changes. Use Local Agent for Mac-local builds/toolchains, simulator execution, serial/USB/flash and physical-board observation. Never launch local Codex from a Local Agent task.

## Roadmap order

1. Diagnose and fix the SCD41 blocker.
2. Verify real sensor values reach the display and correct the display snapshot validity policy.
3. Port the proven Clay/menu/button/simulator stack from LiteGraph using an appropriately authorized source context.
4. Re-run full display/runtime/memory/hardware qualification and update the exact final SHA in the handoff.
5. Only after the e-ink task is fully closed, resume Controller behavior quality work.
