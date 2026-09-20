# Display UI / Clay port contract

Status: active; Phase 4 is the next implementation stage.

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

### Phase 4 — real dirty-region partial transfer — NEXT

Adapt the donor dirty-region algorithm into growbox refresh planning.

Required behavior:

- compute visible pixel-writing bounds;
- add bounded padding;
- clamp/alignment for SSD1680 constraints;
- union current region with previous dirty region so disappeared/moved pixels are cleared;
- only then narrow SSD1680 RAM-window transfer.

Important: the current partial-refresh waveform path still streams the full 4736-byte frame. The goal here is a true reduced RAM-window transfer, not merely selecting the partial waveform.

Acceptance:

- host tests for region clamp/union/moved-content cleanup;
- full-refresh fallback remains available;
- no stale pixels after moving/removing content;
- measured transferred bytes/region are observable in diagnostics;
- do not introduce a second framebuffer, refresh owner or Clay-to-hardware dependency.

### Phase 5 — native buttons

Implement a growbox-owned ESP-IDF GPIO input layer using semantic events such as Home, Back, Previous, Next and Ok.

Do not copy the Arduino polling driver directly. Reuse only event semantics and long-press intent.

Acceptance:

- host-testable debounce state machine;
- stable-edge debounce rather than donor post-edge lockout behavior;
- long-press timing covered by tests;
- physical GPIO layer does not own navigation state.

### Phase 6 — menu/settings mechanics

Port only generic stack/selection/scroll/confirmation mechanics needed by real growbox operator use cases.

Any setting/action that can affect control must call the existing growbox domain/application API. UI must never become an output transport or safety owner.

### Phase 7 — hardware qualification

After software gates are green, qualify the exact candidate SHA on `/dev/cu.usbserial-1130` with outputs fenced unless the operator explicitly authorizes actuation.

Verify:

- full/partial refresh;
- navigation/buttons;
- page correctness;
- no display-induced SCD41/control-loop regression;
- no panic/watchdog/brownout;
- heap/PSRAM stability;
- display worker stack high-water mark;
- ghosting and periodic full-refresh policy;
- retry/failure behavior.

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
4. continue from the first incomplete phase above; currently Phase 4;
5. preserve the shared `growbox_display_model` boundary and existing `DisplayNavigation` ownership;
6. preserve the backend-owned single framebuffer, async transaction and C++17/C++20 boundary;
7. do not reopen SSD1680 pin mapping, rotation, async ownership or SCD41 recovery without new evidence.
