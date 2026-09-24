# Stabilization handoff

Updated: 2026-09-21
Repository: `MichalMatu/bloomml`
Canonical source branch: `main`
Control branch: `agent-control`

## Purpose

Stop feature expansion and stabilize the two open physical I/O paths before further menu/settings work:

1. CrowPanel five-key input.
2. SCD41 with the sensor physically present.

Display rendering, Clay integration, dirty-window transfer and the bounded read-only chooser remain implemented. Do not redesign those subsystems while stabilizing I/O.

## Button baseline to preserve

Board mapping:

- Home GPIO2
- Back GPIO1
- Previous GPIO6
- Next GPIO4
- OK GPIO5

Electrical/semantic contract:

- input with pull-up;
- active LOW;
- sampler period 20 ms in the growbox implementation;
- stable-edge debounce 40 ms in the growbox implementation;
- long press 700 ms;
- short press is emitted only after stable release;
- a long press emits once and suppresses the later short press;
- GPIO sampler owns no navigation/control/output state.

A known-good CrowPanel reference uses the same GPIO mapping and active-low pull-up behavior. Treat any physical mismatch as an implementation/board-integration problem, not a reason to add more UI state.

## Button qualification gate

Before any further menu/settings feature:

- boot an exact known SHA with real outputs/RF/thermal test sequence disabled;
- prove one short press from each of the five physical keys;
- prove OK long-press once;
- verify no duplicate short event after that long press;
- prove Home/Back/Previous/Next/OK produce the intended semantic button IDs;
- prove queue drops remain zero during normal operator use;
- prove display navigation/chooser state changes only in the existing display runtime owner;
- capture UART evidence and record the exact SHA and port.

Startup `eink_buttons_ready=1` is necessary but not sufficient.

## SCD41 baseline to preserve

The app-level source is `src/climate/input/sensors/Scd41InsideSource.*`.

Startup sequence:

`wake_up -> stop_periodic_measurement -> reinit -> start_periodic_measurement`

Runtime behavior:

- the main runtime ticks every 1 second;
- each tick may poll `scd4x_get_data_ready_status()`;
- `scd4x_read_measurement()` runs only when new sensor data is ready;
- the last valid sample is cached with age;
- one bounded liveness recovery is attempted after 30 seconds without progress.

Therefore a fresh sample roughly every five seconds is compatible with normal SCD41 periodic operation. Do not fix that by forcing reads every second.

## SCD41 qualification gate

With the sensor physically attached:

- I2C probe at 0x62 must succeed;
- periodic start must succeed;
- observe repeated fresh CO2 measurements over a bounded run;
- verify sample age resets on fresh readings and grows only while using cache;
- inspect `readErrorCount`, `invalidMeasurementCount`, `successfulMeasurementCount`, and `recoveryAttemptCount`;
- no recurring recovery loop or repeated I2C/read failure;
- record exact SHA, port and UART evidence.

Only then mark the SCD41 path closed again.

## Branch policy

Keep only:

- `main`
- `agent-control`
- `gh-pages`

Temporary `agent/*` branches are disposable after their work is integrated or superseded. Durable project state belongs in `main` documentation, not abandoned task branches.

## Next implementation rule

No new settings framework, menu expansion, alternate button owner, second display worker/framebuffer, or SCD41 polling redesign until both physical gates above pass.
