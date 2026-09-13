# New chat prompt — growbox e-ink + SCD41

Use this prompt as the first user message in the next chat.

```text
[LA_AGENT=815cf40f-8d2a-4e1f-b7cc-c0f4e37b6cb5]
[LA_REPO=growbox-ml-controller]
[LA_REPOSITORY=MichalMatu/growbox-ml-controller]

Hard binding is immutable. Work only on MichalMatu/growbox-ml-controller. Follow docs/OPERATIONS.md and docs/AUTONOMOUS_CHAT_LOOP.md. Never inspect/execute work for another repository from this chat.

Read first:
  docs/EINK_SCD41_CONTINUATION_HANDOFF_20260913.md
  docs/CURRENT_STATUS.md

Current branch:
  feature/eink-clay-status
Current HEAD when handoff was written:
  01db8228e6d822b5c64359abd8bf2d85341b5919
Authorized board:
  /dev/cu.usbserial-1130
Never use:
  /dev/cu.usbserial-10
Do not use /dev/cu.usbserial-1120 without separate authorization.

PRIMARY TASK — DEBUG SCD41 FIRST

Physical CrowPanel e-ink already works. It is rotated 180 degrees, the large “Growbox Status” header is removed, and display refresh runs asynchronously. Do not redo that work.

Current physical symptom: the e-ink shows no sensor values and a warning `!`.

UART after >130 s:
  scd_available=1
  scd_sample=0
  scd_read_errors=0
  scd_invalid=0
  scd_samples=0
while:
  tp_sample=1
  xiaomi_sample=1
  rtc_available=1
  rtc_trusted=1
  SD remains mounted and logging works.

History audit did NOT find an obvious functional regression in Scd41InsideSource. Its measurement implementation predates the e-ink work; 2026-09-12 commit 3ab26a98b mainly grouped/moved the source. I2C remains SDA=21 / SCL=38. TelemetryReporter still calls scd41_.sample() every report.

DO THIS IN ORDER

1. Run the existing service-console `sensors` command on the already flashed board. It calls the same scd41_.sample() path and exposes validity, available, successful reads, errors and invalid counts.
2. Capture COMPLETE boot evidence from /dev/cu.usbserial-1130, including I2C probe, SCD41 begin/start_periodic_measurement messages and early telemetry.
3. If samples remain zero, add minimal diagnostics for:
   - scd4x_start_periodic_measurement() return code
   - scd4x_get_data_ready_status() return code + ready bit
   - scd4x_read_measurement() return code if reached
4. Identify the exact regression/root cause before changing behavior. Prefer a known-good pre-eink executable or minimal SCD41 diagnostic comparison on the same board when useful.
5. Make the smallest verified fix and add a regression test.
6. Build/flash exact candidate with these all disabled:
   GROWBOX_RF433_LOOPBACK_ENABLED=0
   GROWBOX_STAGE28_REAL_OUTPUTS_ENABLED=0
   GROWBOX_STAGE28_THERMAL_TEST_SEQUENCE_ENABLED=0
7. Verify scd_samples increases and real values reach the display.
8. Then improve display snapshot semantics so healthy authoritative non-SCD inputs can still be shown without hiding a genuine SCD41 fault.

SECOND TASK — CLAY PORT, ONLY AFTER SCD41 IS CLOSED

The intended reference is the separate local project:
  /Users/michal/Documents/PlatformIO/Projects/esp32s3_LiteGraph

Reuse, rather than recreate:
- Clay menu/model/layout/navigation
- Clay view/controller/page structure
- physical button debounce/navigation
- glance/status/time modules where useful
- ActionConfirmationDialog where useful
- refresh policy / Clay view-mode manager
- Clay menu/display simulator + event model
- host-side Clay tests

Do not copy unrelated LiteGraph application/Wi-Fi/settings ownership. Growbox UI remains a read-only snapshot/presenter and never executes RF433 or owns safety/output state.

BINDING LIMIT: this chat is hard-bound to growbox-ml-controller. Do not ask this Local Agent to inspect the LiteGraph repository. Use a separately authorized repo context or explicitly transferred source files/snippets for that analysis.

DO NOT REOPEN without evidence:
- CrowPanel pin map
- SSD1680 transport/backend
- physical display initialization
- async display worker
- 180-degree rotation
- removal of large title
- SSD1680 partial command 0xFC
- completed display host suites / Stage27C build path

Every Local Agent task must keep the exact agent_binding above. Use Local Agent for Mac-local build/serial/USB/device work; direct GitHub for small bounded repo edits; never launch local Codex.

Completion of this continuation requires: SCD41 root cause identified, minimal fix + regression test committed, exact image flashed to /dev/cu.usbserial-1130 with outputs disabled, UART proving real SCD41 measurements, e-ink showing real values, documentation updated. Only then start Clay/menu/button/simulator port.
```
