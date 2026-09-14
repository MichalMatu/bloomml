# Current controller status

Updated: 2026-09-14
Repository: `MichalMatu/growbox-ml-controller`
Canonical source branch: `main`
Control branch: `agent-control`
Publishing branch: `gh-pages`

## Current phase

The CrowPanel e-ink/SCD41 recovery work is closed for normal development purposes. The final code-bearing closeout commit is:

`c720c0a1d6d6d9c80daeb04b2dc69efa53d2c8a4`

It keeps the Sensirion-aligned SCD41 clean-start sequence:

`wake_up -> stop_periodic_measurement -> reinit -> start_periodic_measurement`

and adds one bounded runtime liveness recovery. If no new SCD41 measurement appears for 30 seconds, firmware performs that same recovery sequence at most once per MCU boot. It does not restart the ESP32 and it does not retry indefinitely.

The historical SCD41 failure remains classified as intermittent retained-device/startup state. It has been reproduced on one boot and absent on another boot of the same firmware/hardware, so do not claim the physical root cause is mathematically eliminated. The production behavior is now bounded and self-recovering once per boot instead of silently remaining stale forever.

## Hardware qualification evidence

Authorized growbox serial device: `/dev/cu.usbserial-1130`.

Never use `/dev/cu.usbserial-10`.

Do not use `/dev/cu.usbserial-1120` without separate authorization.

The apparent missing-board incident on 2026-09-14 was traced to a loose USB cable. Once corrected, `/dev/cu.usbserial-1130` enumerated normally. The protected `/dev/cu.usbserial-10` device was not opened or reset during diagnosis.

### Proven clean-start/e-ink baseline

Exact SHA `a92074b74b055c58c0949c7c38f4896638ebf227` passed a 90-second hardware qualification with physical outputs fenced off and e-ink enabled:

- `eink_requested=1`, `eink_ready=1`;
- 5 successful physical e-ink refreshes;
- 8 consecutive SCD41 samples;
- `scd_read_errors=0`;
- `scd_invalid=0`;
- zero panic events;
- zero brownout events;
- no unexpected reboot.

A bounded 5-cycle MCU-reset stress test then passed 5/5 cycles. Every cycle reached a valid SCD41 sample, had e-ink enabled/ready, completed a physical refresh, and recorded no panic or brownout.

### Intermittent failure reproduction

On later exact SHA `f53979923ad47dd9422f5457e8b54596c7fdf093`, one boot produced a valid first SCD41 measurement and then stalled with `scd_samples=1` while sample age grew past 70 seconds. `scd_read_errors` and `scd_invalid` stayed zero, e-ink continued refreshing and there was no panic/brownout. A subsequent boot of the same SHA sampled normally (`1 -> 2 -> ... -> 7`).

That A/B evidence is why `c720c0a1...` adds the one-shot 30-second liveness recovery.

### Final `c720c0a1...` qualification state

For `c720c0a1d6d6d9c80daeb04b2dc69efa53d2c8a4`:

- relevant pre-commit gates passed;
- SCD41 policy/regression tests passed;
- all five display host suites passed;
- full CrowPanel ESP-IDF build with e-ink enabled passed;
- GitHub CI #1023 passed `web-tests`, `host-tests` and `esp-idf-build`.

A fresh physical flash/monitor gate was attempted, but `/dev/cu.usbserial-1130` was not enumerated at that moment, so the task stopped before flash and touched no device. Therefore the exact `c720...` binary does not yet have a fresh hardware run. This is a hardware-availability limitation, not a failed firmware qualification.

## E-ink status

The CrowPanel 2.9-inch DIE01129S001 e-paper display path is operational and observer-only.

Hardware/profile facts:

- ESP32-S3-WROOM-1 N8R8 / 8 MB PSRAM;
- 128x296 SSD1680Z e-paper;
- SCLK 12, MOSI 11, CS 45, DC 46, RST 47, BUSY 48;
- display power GPIO 7;
- shared I2C SDA 21 / SCL 38.

The operator model remains four pages:

1. Environment / main quick-glance page;
2. Outputs;
3. System;
4. Diagnostics.

The first page again renders the full quick-glance set instead of hiding useful information: temperature, RH, CO2, SCD41 freshness, time, controller mode, lamp, fan, humidifier and safety. The other pages remain expansions rather than replacements.

Completed and tested:

- display initialization and refresh;
- 180-degree rotation;
- Clay/presenter/render-list seam;
- observer-only display ownership;
- asynchronous display worker outside the controller hot path;
- partial refresh path;
- display host suites are part of canonical CI;
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

Long-lived branches are intended to be only:

- `main` — source and documentation;
- `agent-control` — Local Agent control plane;
- `gh-pages` — published web output.

Temporary implementation branches should be deleted after their work is incorporated and verified. Three temporary closeout safety refs created during this session contain no unique work and should be removed administratively: `tmp/close-display-scd41`, `tmp/close-display-scd41-backup`, and `tmp/close-display-scd41-safety`.

Detailed historical phase handoffs/plans are intentionally kept in Git history instead of live `docs/`. Compact milestone evidence is in `docs/HISTORY.md` and `docs/CHANGELOG.md`.

## Next work

1. Treat `c720c0a1...` as the closed code baseline for the e-ink/SCD41 recovery work.
2. Continue the next planned display/menu/button/simulator stage without reopening completed backend work unless new evidence requires it.
3. When `/dev/cu.usbserial-1130` is available again, a bounded physical confirmation of the exact or descendant firmware can validate the one-shot recovery on hardware; this is not a prerequisite for continuing unrelated development.
4. Keep the SCD41 clean-start and one-shot recovery regression tests intact.
