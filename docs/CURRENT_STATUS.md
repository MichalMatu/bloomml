# Current controller status

Updated: 2026-09-14
Repository: `MichalMatu/growbox-ml-controller`
Canonical source branch: `main`
Control branch: `agent-control`
Publishing branch: `gh-pages`

## Current phase

Repository structure and documentation are being consolidated before debugging resumes. The active technical blocker remains the **SCD41 measurement regression** discovered after the first working CrowPanel e-ink integration.

The e-ink hardware path itself is working and should not be reimplemented.

## Exact known-good/current evidence

Latest code-bearing display/debug baseline that was built and flashed during the e-ink work:

`01db8228e6d822b5c64359abd8bf2d85341b5919`

The later branch head before repository cleanup was:

`ceaaa88801568cccf2f1cbc86021ce3b2b2809d5`

Its latest commits only updated the SCD41 continuation/status documentation; the active behavior problem is unchanged.

Authorized growbox serial device: `/dev/cu.usbserial-1130`.

Never use `/dev/cu.usbserial-10`.

Do not use `/dev/cu.usbserial-1120` without separate authorization.

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
- real runtime data reaches the display snapshot path when valid.

Do not reopen the panel pin map, SSD1680 backend, rotation or async architecture unless new evidence directly requires it.

## Active SCD41 blocker

The display currently cannot show valid environment values because its snapshot validity depends on the SCD41 sample, while the SCD41 source reports availability without ever recording a successful measurement.

Stable UART evidence captured during the handoff:

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

This points to the SCD41 measurement/start/readiness path rather than a general I2C, RTC, SD or display failure.

## Debugging sequence

1. Run the existing service-console `sensors` command on the already flashed board.
2. Capture complete boot evidence for I2C probe, SCD41 `begin()`, periodic-measurement start and early telemetry.
3. If samples remain zero, add only bounded diagnostics for return codes / ready state around `scd4x_start_periodic_measurement`, `scd4x_get_data_ready_status` and `scd4x_read_measurement`.
4. Compare with a known-good pre-regression executable or a minimal SCD41-only diagnostic on the same board if needed.
5. Identify the root/regression boundary before changing behavior.
6. Implement the smallest fix and a regression test.
7. Build, flash and verify the exact candidate on `/dev/cu.usbserial-1130` with physical outputs disabled.
8. After SCD41 is healthy, improve display fallback semantics if useful without fabricating CO2 or hiding a real sensor fault.

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

## Next work after SCD41

1. Confirm real environment values on the display.
2. Continue the bounded Clay/menu/button/simulator port from the proven LiteGraph implementation using an appropriately bound source context.
3. Complete display/runtime/memory/hardware qualification.
4. Only then resume controller behavior-quality tuning from `docs/PROJECT_ROADMAP.md`.
