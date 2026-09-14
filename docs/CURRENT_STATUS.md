# Current controller status

Updated: 2026-09-14
Repository: `MichalMatu/growbox-ml-controller`
Canonical source branch: `main`
Control branch: `agent-control`
Publishing branch: `gh-pages`

## Current phase

Repository cleanup and the CrowPanel SCD41/e-ink recovery investigation are complete enough to resume normal development.

The previous active SCD41 blocker is no longer reproduced on the qualified candidate. The current code-bearing recovery candidate is:

`a92074b74b055c58c0949c7c38f4896638ebf227`

It adds the Sensirion-aligned SCD41 clean-start sequence:

`wake_up -> stop_periodic_measurement -> reinit -> start_periodic_measurement`

Do not claim the historical intermittent fault is mathematically impossible to recur; it was intermittent. The candidate has, however, passed bounded hardware qualification including repeated MCU-only resets with the sensor left powered.

## Hardware qualification evidence

Authorized growbox serial device: `/dev/cu.usbserial-1130`.

Never use `/dev/cu.usbserial-10`.

Do not use `/dev/cu.usbserial-1120` without separate authorization.

The apparent missing-board incident on 2026-09-14 was traced to a loose USB cable. Once corrected, `/dev/cu.usbserial-1130` enumerated normally. The protected `/dev/cu.usbserial-10` device was not opened or reset during diagnosis.

### Pre-fix baseline observation

The previously flashed display baseline `01db8228e6d822b5c64359abd8bf2d85341b5919` was passively observed for 45 seconds after the cable was corrected:

- one normal POWERON boot;
- zero panic/brownout events;
- SCD41 produced a valid sample in that boot;
- e-ink refreshed successfully.

This confirms the historical failure was intermittent rather than a deterministic source regression.

### `a92074b...` qualification

With physical outputs fenced off, exact SHA `a92074b74b055c58c0949c7c38f4896638ebf227` was built/flashed to `/dev/cu.usbserial-1130` and observed with the CrowPanel e-ink path enabled.

90-second qualification result:

- `eink_requested=1`, `eink_ready=1`;
- 5 successful physical e-ink refreshes;
- 8 consecutive SCD41 samples;
- `scd_read_errors=0`;
- `scd_invalid=0`;
- zero panic events;
- zero brownout events;
- no unexpected reboot.

A bounded 5-cycle MCU-reset stress test then passed 5/5 cycles. Every cycle reached a valid SCD41 sample, had e-ink enabled/ready, completed a physical refresh, matched firmware SHA `a92074b...`, and recorded no panic or brownout.

Local Agent evidence tasks:

- `20260914-scd41-ab-after-cable-fix-v1`
- `20260914-scd41-eink-enabled-qual-v1`
- `20260914-scd41-mcu-reset-stress-v1`

The canonical GitHub CI run for the code candidate also completed successfully across `web-tests`, `host-tests`, and `esp-idf-build`.

## E-ink status

The CrowPanel 2.9-inch DIE01129S001 e-paper display is operational.

Hardware/profile facts:

- ESP32-S3-WROOM-1 N8R8 / 8 MB PSRAM;
- 128x296 SSD1680Z e-paper;
- SCLK 12, MOSI 11, CS 45, DC 46, RST 47, BUSY 48;
- display power GPIO 7;
- shared I2C SDA 21 / SCL 38.

Completed and physically observed:

- display initialization and refresh;
- 180-degree rotation;
- compact status layout;
- observer-only display ownership;
- asynchronous display worker outside the controller hot path;
- partial refresh path;
- real runtime values coexist with SCD41 sampling.

Do not reopen the panel pin map, SSD1680 backend, rotation, async architecture or the discarded display-brownout hypothesis unless new evidence directly requires it.

## Safety/output invariants

- deterministic Rule control remains authoritative;
- ML remains shadow/research-only;
- `OutputSupervisor` remains the only normal configured-output owner;
- UI never executes RF433 directly or mutates safety state;
- unavailable transport never becomes fake physical success;
- thermal safety remains authoritative;
- physical outputs stay disabled during unattended debugging/qualification;
- display work must not block the 1-second control loop.

Unattended fence values remain equivalent to:

```text
GROWBOX_RF433_LOOPBACK_ENABLED=0
GROWBOX_STAGE28_REAL_OUTPUTS_ENABLED=0
GROWBOX_STAGE28_THERMAL_TEST_SEQUENCE_ENABLED=0
```

## Repository policy after cleanup

Long-lived branches are only:

- `main` — source and documentation;
- `agent-control` — Local Agent control plane;
- `gh-pages` — published web output.

Temporary implementation branches should be deleted after their work is incorporated and verified.

Detailed historical phase handoffs/plans are intentionally kept in Git history instead of live `docs/`. Compact milestone evidence is in `docs/HISTORY.md` and `docs/CHANGELOG.md`.

## Next work

1. Keep the SCD41 recovery regression test and exact hardware evidence intact.
2. Continue the bounded Clay/menu/button/simulator port from the proven LiteGraph implementation using an appropriately bound source context.
3. Complete display/runtime/memory/hardware qualification where still missing.
4. Resume controller behavior-quality tuning from `docs/PROJECT_ROADMAP.md` only after the remaining display/UI work is stable.
