# Display UI / Clay port contract

Status: active; Phases 0-6 are complete. Phase 7 has autonomous exact-SHA hardware evidence; manual operator checks remain.

Documentation re-audit baseline: `growbox-ml-controller@6d083e5b00a29b20a8a9bb6f2bb83a395634aff5`.
Donor snapshot: `esp32s3_LiteGraph@5b8c758c365547ddeaab65bbe9f849bdd071695d` under `vendor/litegraph_epd_port/`.

## Goal

Upgrade the already-working CrowPanel 2.9-inch operator display with the useful Clay layout/menu/button/simulator mechanics from LiteGraph without replacing the native growbox display backend, controller ownership, output truth or safety architecture.

This is a bounded UI/layout port, not a display-driver rewrite.

## Frozen growbox baseline

Keep these existing decisions unless new evidence requires a change:

- native ESP-IDF firmware;
- ESP32-S3 CrowPanel 2.9-inch, 296x128 monochrome SSD1680 path;
- one fixed 296x128 1-bpp framebuffer (`4736` bytes);
- existing native SSD1680 backend and frame mapping;
- asynchronous display worker outside the control hot path;
- observer-only `DisplaySnapshot` -> presenter -> render path;
- four operator pages: Environment, Outputs, System, Diagnostics;
- `OutputSupervisor` remains the only normal configured-output owner;
- display/UI code never writes RF433 or safety/control state directly;
- production main component remains C++17 unless there is a separately justified migration.

## Vendor snapshot

`vendor/litegraph_epd_port/` is a read-only adaptation source. It is intentionally excluded from firmware and host build include paths.

Useful donor mechanics captured there:

- Clay 0.14 source pin;
- `ClayRenderer`, `ClipStack`, theme/text/layout helpers;
- `ClayRenderEngine` and current/previous dirty-region union algorithm;
- scroll state and navigation/controller mechanics;
- menu model/layout/preview patterns;
- input-event semantics and Arduino reference driver;
- SDL 296x128 simulator/font-rendering references;
- generic host renderer/clipping tests.

Do not port wholesale:

- GxEPD2 or alternate panel backends;
- Arduino hardware ownership;
- LiteGraph Wi-Fi/Nodeflow views and presenters;
- touch/TFT/EPD47/ST7262 paths;
- the full generic worker-lifecycle framework;
- direct UI-to-output actions;
- donor `Memory.h` implementation or PSRAM wrapper hierarchy.

## Required language/build boundary

The bundled Clay 0.14 header requires C++20. Current growbox `src` explicitly compiles as C++17.

Current integration:

```text
C++17 growbox domain/runtime
        |
        | growbox-owned semantic structs only
        v
isolated growbox_clay_ui component (C++20)
        |
        | render into caller-provided storage
        v
backend-owned 296x128 monochrome framebuffer
        |
        v
existing async display service / SSD1680 backend
```

Rules:

- do not include `clay.h` from normal `src/` public headers;
- no `Clay_*` type may cross the component public boundary;
- use growbox-owned semantic structs for page/view input;
- use growbox-owned plain frame/result types at the boundary;
- keep the rest of the firmware C++17;
- use native ESP-IDF `heap_caps_*` for the Clay arena rather than donor Arduino/PSRAM helpers;
- the SSD1680 backend remains the sole owner of the 4736-byte framebuffer;
- Clay receives mutable framebuffer access only during an open backend frame transaction and must not retain that pointer.

The minimal shared semantic model lives in `lib/growbox_display_model/`. Keep it behavior-free: it is a C++17 DTO boundary, not a new UI framework or state owner.

## Production render path

```text
runtime truth
   -> DisplaySnapshot
   -> DisplayPresenter / DisplayPageModel
   -> growbox Clay layouts (C++20 component)
   -> monochrome rasterization
   -> existing backend-owned 296x128 framebuffer
   -> existing async display transaction/service
   -> existing SSD1680 backend
```

The semantic presenter remains authoritative for what values/statuses are displayed. Clay owns layout, clipping and rasterization only. Navigation, transaction generation, refresh confirmation, retry behavior and hardware stay in the growbox display layer.

## Port phases

### Phase 0 — source preparation — DONE

- curated vendor snapshot created;
- donor commit pinned;
- Clay 0.14 header blob pinned;
- renderer/menu/controller/dirty-region references captured;
- SDL/host-test sources pinned or copied;
- vendor remains outside all builds.

### Phase 1 — isolated Clay component + host simulator — DONE

The isolated C++20 component and exact 296x128 host simulator were implemented without firmware integration.

Acceptance evidence:

- exact Clay 0.14 source copied from the lock pin;
- standalone C++20 component and C++17 public-boundary compile checks pass;
- exact 296x128 1-bpp simulator emits 4736 framebuffer bytes;
- renderer/clipping and simulator smoke tests pass;
- no firmware behavior changed in this phase.

### Phase 2 — reproduce current four pages in simulator — DONE

Environment / Outputs / System / Diagnostics are rendered from the current growbox semantic `DisplayPageModel` through real Clay 0.14 in the exact 296x128 host framebuffer.

Implemented boundary:

- `DisplayPresenter` remains the authority for page content and warning meaning;
- `lib/growbox_display_model/` carries only the bounded semantic page DTO;
- `growbox_clay_ui::renderHostPage()` consumes that DTO without depending on application-layer `src/` headers;
- existing `DisplayNavigation` remains the navigation owner;
- the monochrome text path uses a readable 5x7 glyph renderer with matching Clay text measurement;
- each render transaction resets the Clay current context before/after its local arena lifetime.

Historical Phase 2 nominal hashes on `33e20713b605d93e97fadd7b2c1af8748b3bf897` used the original host-only bit polarity:

- Environment / `Growbox status`: `2d40c7bccbf2e12b`;
- Outputs: `b7e3999ea15d8d2b`;
- System: `430713d76487fc25`;
- Diagnostics: `3be1346eb2b8c1e5`.

Verification:

- Local Agent `20260920-clay-phase2-v2`: dedicated display/Clay suites PASS, full `make test-host` PASS, ESP-IDF build PASS, `git diff --check` PASS;
- Local Agent `20260920-clay-phase2-goldens-v1`: pinned Phase 2 golden hashes PASS.

No physical-display claim was made by Phase 2.

### Phase 3 — connect Clay renderer to existing framebuffer/service — DONE

The Phase 2 layout is now the production renderer behind the existing asynchronous display transaction.

Implemented boundary:

- shared `PageRenderer` is used by both host simulation and firmware;
- the C++20 Clay implementation accepts a caller-owned arena and caller-provided framebuffer rather than owning either resource;
- `CrowPanelDisplayService` allocates one persistent Clay scratch arena in PSRAM during startup and reuses it for refreshes;
- `CrowPanelSsd1680DisplayBackend` remains the sole owner of the one 4736-byte framebuffer and exposes it only while its frame transaction is open;
- the existing display worker remains the only slow-work owner; Clay did not create another task;
- `DisplayAsyncTransaction`, generation matching, `confirmRendered()` and retry semantics remain in the growbox display layer;
- failed Clay/frame/backend transactions call `cancelFrame()` and do not falsely advance observer state;
- full and partial refresh planning continues through the existing SSD1680 backend; true dirty-window transfer is intentionally deferred to Phase 4;
- no navigation, control, safety or hardware ownership moved into Clay.

During integration the host framebuffer polarity was corrected to match the production SSD1680/raster contract: a cleared bit is black and a set bit is white (`0 = black`, `1 = white`). PBM export performs the format-specific inversion. This changed byte hashes but not layout or black-pixel counts.

Production-polarity nominal hashes:

- Environment / `Growbox status`: `3bb469ff366c69bf` — 2270 black pixels;
- Outputs: `2e43841abef23957` — 2416 black pixels;
- System: `262017750c71fa89` — 2453 black pixels;
- Diagnostics: `c22a38670d9064f1` — 2321 black pixels.

All four still emit 25 Clay render commands in the nominal integration fixture.

Acceptance evidence:

- async worker remains slow-work owner: PASS;
- `confirmRendered`/success semantics preserved: PASS by transaction/bridge tests and code path;
- failure/retry does not falsely advance observer state: PASS;
- no second framebuffer owner: PASS; Clay writes the backend-owned buffer during an open transaction;
- no per-refresh Clay heap allocation in firmware: PASS; arena is allocated once in PSRAM at display-service startup;
- current full-refresh path still works architecturally before dirty-region optimization: PASS by real production build and backend contract;
- production C++17 / Clay C++20 boundary compiles: PASS.

Verification:

- Local Agent `20260920-clay-phase3-v3`: format PASS, dedicated display/Clay suites PASS, full 50-test `make test-host` PASS and generic ESP-IDF build PASS;
- Local Agent `20260920-clay-phase3-polarity-goldens-v1`: production-polarity hashes and unchanged black-pixel counts captured;
- Local Agent `20260920-clay-phase3-crowpanel-build-v4` on exact source `76447b963bb1e3f4d9290bfb26380ac30a0a41ab`: canonical `stage27c_crowpanel.sh` real-input/N8R8/e-ink build PASS, including `CrowPanelDisplayService.cpp` and `ClimateV6RealInputRuntime.cpp`; image binary `0xcc970`, smallest app partition `0x400000`, 80% free; `idf.py size` total image size 837885 bytes; clean tree and `git diff --check` PASS.

`make build-crowpanel` is the canonical production display build gate. Generic `make build` is not sufficient for Clay/display integration because its default app mode does not compile the CrowPanel real-input path.

This remains software/build evidence only. No new physical CrowPanel qualification is claimed by Phase 3.

### Phase 4 — real dirty-region partial transfer — DONE

The production SSD1680 partial-refresh path now performs a true reduced RAM-window transfer rather than selecting the partial waveform while streaming the full 4736-byte plane.

Implemented boundary:

- `growbox_clay_ui::planPageDirtyRegion()` compares the last physically successful semantic `DisplayPageModel` with the next page and marks only changed header/row footprints;
- renderer and dirty planner share private Clay page geometry constants, so dirty bounds cannot silently drift from the actual layout;
- changed and removed rows include their full old/new row footprint, preventing stale pixels when text disappears or changes width;
- the SSD1680 backend owns hardware padding/clamping and logical-to-native 128x296 byte-window mapping;
- partial refresh writes only the selected native window to current RAM, activates the partial waveform, then mirrors that same window to previous RAM;
- full refresh remains the fallback whenever controller previous-RAM state is not known/seeded;
- failed physical refreshes do not advance the service's last-physical-page baseline; hardware reinitialization clears the previous-RAM seed and therefore forces a full refresh on retry;
- there is still exactly one 4736-byte framebuffer, one display worker and one physical refresh owner; Clay did not gain hardware ownership.

Host acceptance evidence:

- clamp/expand/union and both SSD1680 rotations: PASS;
- changed/removed semantic row planning: PASS;
- one changed row `{x=6,y=44,w=284,h=9}` plus 16 px hardware padding maps to exactly 1776 bytes per SSD1680 RAM plane, below the full 4736-byte plane: PASS;
- full `scripts/test_display_host.sh`: PASS;
- full `make test-host`: 50/50 PASS;
- canonical `make build-crowpanel`: PASS;
- `git diff --check`: PASS.

Physical qualification on merged exact SHA `4c32466c6a0af798620faf0e85416ad39ea4f07a` used only authorized `/dev/cu.usbserial-120`, with `GROWBOX_STAGE28_REAL_OUTPUTS_ENABLED=0`, `GROWBOX_RF433_LOOPBACK_ENABLED=0` and the e-ink path enabled. The 120-second UART capture showed:

- firmware-reported SHA `4c32466c6a0af798620faf0e85416ad39ea4f07a`;
- initial full refreshes with `window_bytes=4736`, `physical_partial=0`;
- a real partial refresh at uptime ~73.6 s with `dirty=0,48,296,41`, native window `6-11,0-295`, `window_bytes=1776`, `ram_payload_bytes=3552`, `physical_partial=1`;
- display worker `stack_min_free_bytes=2228`;
- no Guru Meditation, brownout, task/interrupt watchdog timeout, assert, abort or backtrace in the captured interval.

The log's recurring `INFO/WATCHDOG ... heartbeat` lines are application heartbeat diagnostics, not watchdog failures.

Local Agent evidence: `20260920-phase4-hardware-qualification-v1` performed the exact-SHA flash and UART capture; its over-broad first-pass text filter failed on the benign word `WATCHDOG`. `20260920-phase4-hardware-qualification-v2` re-parsed the same captured log with specific crash signatures and passed, preserving the physical partial-refresh evidence above.

### Phase 5 — native buttons — DONE

The growbox now owns a native ESP-IDF button input path for the CrowPanel 2.9-inch board. The donor Arduino driver was not copied; only key semantics and long-press intent were retained.

Implemented boundary:

- board mapping is Home GPIO2, Back GPIO1, Previous GPIO6, Next GPIO4 and OK GPIO5, active-low with pull-ups;
- `DisplayButtonStateMachine` is pure and host-testable, with 40 ms stable-edge debounce that restarts the candidate timer on every raw transition;
- short press is emitted only after a stable release; a 700 ms long press emits once and suppresses the later short-press event;
- `CrowPanelButtonInput` samples five GPIOs every 20 ms from `esp_timer` and pushes semantic events into a static FreeRTOS queue of depth 8;
- no dedicated button task or stack was added;
- the sampler does not own navigation, display refresh, control, outputs or safety;
- the main runtime task drains semantic events before the 1-second coordinator tick and is the only path that calls `DisplayTelemetryObserver::handleButton()`;
- long-press events are currently diagnostic/semantic only; Phase 5 intentionally does not invent a menu/settings action for them;
- timer/runtime lifecycle state is atomic so the ESP timer callback does not race the runtime task.

Verification:

- Local Agent `20260920-phase5-buttons-v3`: focused button tests PASS, full display/Clay suites PASS, full `make test-host` 50/50 PASS, canonical `make build-crowpanel` PASS, `git diff --check` PASS;
- final lifecycle review `20260920-phase5-buttons-v4`: the same gates PASS after atomic sampler lifecycle hardening;
- firmware binary was `0xcdb30` bytes with 80% free in the smallest `0x400000` app partition;
- PR #12 merged as exact source `7533f7f4c2c92675ef0908c91912b89a66f7ee4a`.

Merged exact-SHA hardware startup qualification on authorized `/dev/cu.usbserial-120` kept real outputs, RF loopback and the thermal sequence disabled. Local Agent `20260920-phase5-merged-hardware-smoke-v2` captured the boot from reset and confirmed:

- firmware SHA `7533f7f4c2c92675ef0908c91912b89a66f7ee4a`;
- `eink_requested=1`, `eink_ready=1`, `eink_buttons_ready=1`;
- `real_outputs_requested=0`, `real_outputs_ready=0`, `outputs=fake-locked`;
- physical e-ink refreshes continued to succeed, including partial transfer;
- no Guru Meditation, brownout, task/interrupt watchdog timeout, assert, abort or backtrace in the captured interval.

No human physical key press occurred during the unattended capture (`button_events=0`). Therefore Phase 5 claims GPIO initialization and software semantics, not manual key-actuation/navigation qualification. Real physical button presses remain explicitly part of Phase 7.

### Phase 6 — menu/settings mechanics — DONE (bounded read-only scope)

The justified growbox operator scope is a read-only page chooser rather than a donor-style settings subsystem. A write-side settings framework was intentionally not introduced because no existing growbox application/domain mutation required it.

Implemented boundary:

- long-press OK opens a four-item Environment / Outputs / System / Diagnostics chooser;
- Previous / Next moves only the temporary selection; short OK commits through the existing `DisplayNavigation`; Back cancels and Home closes the chooser and returns to Environment;
- `DisplayMenuState` owns only chooser visibility/selection; committed page state remains owned by `DisplayRuntimeController` / `DisplayNavigation`;
- menu rendering still produces the existing neutral `DisplayPageModel`; Clay gains no navigation, control, persistence or hardware ownership;
- warning identity is copied from the underlying authoritative page so safety/clock/storage/supervisor warnings remain visible;
- menu changes invalidate stale planned frames while the existing async render confirmation contract remains authoritative;
- no settings write, output action, RF transport call or safety mutation was added.

Verification:

- Local Agent `20260920-phase6-menu-shell-v1` on exact head `47d37b5e660ddda21da15707d2d8e5381a72c9ed`: clang-format PASS, focused menu tests PASS, full display/Clay suites PASS, full `make test-host` PASS, canonical `make build-crowpanel` PASS and `git diff --check` PASS;
- firmware size was `0xcdea0`, with 80% free in the smallest `0x400000` app partition;
- PR #13 passed GitHub CI including host/clang-tidy and CrowPanel ESP-IDF build, then merged as `b64a7f294da716b59d7ca2a819f938f52ba726b0`.

Phase 6 is complete at this bounded scope. Future writable settings require a concrete operator use case and an existing/explicit application-domain API; they are not implied by the donor UI.

### Phase 7 — hardware qualification — PARTIAL

The autonomous hardware subset has been qualified on exact merged SHA `b64a7f294da716b59d7ca2a819f938f52ba726b0` using only authorized `/dev/cu.usbserial-120`. Real outputs, RF loopback/remote capture and the thermal test sequence were disabled.

Local Agent `20260921-phase7-hardware-qualification-v1` successfully identified the ESP32-S3 with 8 MB PSRAM, built/flashed the exact SHA, and captured 180 seconds of UART. Its final gate intentionally failed because it originally required a live SCD41 sample; the physical probe showed `scd41_0x62=ESP_ERR_NOT_FOUND`. Local Agent `20260921-phase7-hardware-qualification-v2` re-parsed that same physical capture with device-presence-aware acceptance criteria and passed.

Qualified observations from the capture:

- startup: `eink_requested=1`, `eink_ready=1`, `eink_buttons_ready=1`, `real_outputs_requested=0`, `real_outputs_ready=0`, `outputs=fake-locked`;
- display worker: static stack 6144 bytes and persistent Clay PSRAM arena 169792 bytes;
- 18 telemetry records were present; RTC was available/trusted and BLE environment samples were present;
- four successful physical Clay refreshes were observed: two full transfers at 4736 bytes/plane and two reduced partial transfers at 1776 bytes/plane (`dirty=0,48,296,41`, native `6-11,0-295`);
- display worker minimum free stack was 2228 bytes;
- internal free heap remained about 204280 -> 201488 bytes with observed minimum 201488 in the parsed records; PSRAM free remained about 8200868 -> 8188272 bytes with observed minimum 8188272;
- repeated `heap_integrity_ok` records were present; no heap-integrity failure, render failure, confirmation failure or display queue failure was observed;
- no Guru Meditation, brownout, task/interrupt watchdog timeout, assert, abort or backtrace signature was present;
- no physical output actuation was enabled.

Not yet qualified:

- SCD41-specific/control-loop behavior on this run, because the SCD41 device was physically absent/not found;
- manual physical button actuation/navigation, because the unattended capture observed zero button events;
- visual page correctness/ghosting by human inspection;
- the periodic full-refresh cadence after 20 partial refreshes, because the capture contained only two partial refreshes;
- injected physical backend failure/retry behavior (the software transaction/failure path remains covered by host tests).

These remaining checks must not be replaced by synthetic button injection or inferred from host/build success.

## Diagnostics/resource requirements

Track at minimum:

- Clay arena requested/allocated bytes and allocation capability;
- internal RAM and PSRAM free/min/largest values through existing diagnostics;
- display worker stack high-water mark;
- refresh full/partial counts;
- planned and actually transferred region/byte count;
- render failures/retries.

On ESP-IDF/ESP32-S3, `uxTaskGetStackHighWaterMark()` is treated as bytes by the existing growbox diagnostics. Do not import the donor helper that multiplies this value by `sizeof(StackType_t)`.

## Verification order

Use the smallest gate first:

1. host renderer/controller tests;
2. simulator/golden checks;
3. normal host/quality gates affected by the change;
4. `make build-crowpanel` for production display integration;
5. GitHub CI;
6. physical board qualification only for hardware claims.

## Starting a future session

Before implementation:

1. read `../AGENTS.md`, `README.md`, `CURRENT_STATUS.md`, `ARCHITECTURE.md`, this file and `vendor/litegraph_epd_port/README.md`;
2. fetch fresh `main` and verify Local Agent is idle before direct writes;
3. confirm the vendor source pins have not drifted;
4. continue from the first incomplete qualification item above; currently the remaining manual Phase 7 checks;
5. preserve the shared `growbox_display_model` boundary and existing `DisplayNavigation` ownership;
6. preserve the backend-owned single framebuffer, async transaction and C++17/C++20 boundary;
7. do not reopen SSD1680 pin mapping, rotation, async ownership or SCD41 recovery without new evidence.
