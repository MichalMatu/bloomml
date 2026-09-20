# Display UI / Clay port contract

Status: active; Phase 3 is the next implementation stage.

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

Preferred integration:

```text
C++17 growbox domain/runtime
        |
        | plain growbox structs only
        v
isolated growbox_clay_ui component (C++20)
        |
        | Clay commands / internal state stay private
        v
monochrome frame + dirty-region result
        |
        v
existing C++17 display service / SSD1680 backend
```

Rules:

- do not include `clay.h` from normal `src/` public headers;
- no `Clay_*` type may cross the component public boundary;
- use growbox-owned semantic structs for page/view input;
- use growbox-owned plain frame/region/result structs for output;
- keep the rest of the firmware C++17 during the first port;
- use native ESP-IDF `heap_caps_*` for Clay arena allocation rather than donor Arduino/PSRAM helpers.

The minimal shared semantic model has been extracted to `lib/growbox_display_model/`. Keep it behavior-free: it is a C++17 DTO boundary, not a new UI framework or state owner.

## Target render path

```text
runtime truth
   -> DisplaySnapshot
   -> DisplayPresenter / DisplayPageModel
   -> growbox Clay layouts
   -> Clay_RenderCommandArray (inside C++20 component)
   -> monochrome renderer
   -> existing 296x128 framebuffer
   -> existing async display service
   -> existing SSD1680 backend
```

The semantic presenter remains authoritative for what values/statuses are displayed. Clay owns layout, clipping and visual composition only.

## Port phases

### Phase 0 — source preparation — DONE

- curated vendor snapshot created;
- donor commit pinned;
- Clay 0.14 header blob pinned;
- renderer/menu/controller/dirty-region references captured;
- SDL/host-test sources pinned or copied;
- vendor remains outside all builds.

### Phase 1 — isolated Clay component + host simulator — DONE

The isolated C++20 component and exact 296x128 host simulator are implemented without firmware integration.

Acceptance:

- exact Clay 0.14 source copied from the lock pin;
- component builds independently with C++20;
- normal application component remains C++17;
- 296x128 host render target exists;
- renderer/clipping tests pass;
- no firmware behavior changes.

Verification:

- Clay 0.14 header matches pinned Git blob `58006d208f7e58d646578b42524068f44445a4dc` byte-for-byte;
- standalone C++20 component and C++17 public-boundary compile checks pass;
- headless 296x128 1-bpp simulator emits exactly 4736 framebuffer bytes;
- renderer/clipping and simulator smoke tests pass;
- existing display host suites and ESP-IDF firmware build remain green.

### Phase 2 — reproduce current four pages in simulator — DONE

Environment / Outputs / System / Diagnostics are rendered from the current growbox semantic `DisplayPageModel` through real Clay 0.14 in the exact 296x128 host framebuffer.

Implemented boundary:

- `DisplayPresenter` remains the authority for page content and warning meaning;
- `lib/growbox_display_model/` carries only the bounded semantic page DTO;
- `growbox_clay_ui::renderHostPage()` consumes that DTO without depending on application-layer `src/` headers;
- existing `DisplayNavigation` remains the navigation owner and is exercised by the Phase 2 host integration test;
- the host monochrome text path uses a readable 5x7 glyph renderer with a matching Clay text-measurement callback;
- each render transaction clears `Clay_GetCurrentContext()` before and after using its local arena, preventing a Clay context from outliving arena memory during repeated page rendering.

Acceptance evidence:

- same authoritative values/status meanings as current presenter: PASS via presenter-built models used directly by the integration test;
- no LiteGraph-specific domain concepts: PASS;
- deterministic golden/equivalent headless checks: PASS via exact 4736-byte frame hashes;
- navigation model exercised on host: PASS for Next/Previous/Home/Ok/Back behavior;
- current page set usable before settings/menu complexity: PASS with readable host text and all ten semantic rows rendered.

Nominal golden hashes on exact `main` `33e20713b605d93e97fadd7b2c1af8748b3bf897`:

- Environment / `Growbox status`: `2d40c7bccbf2e12b`;
- Outputs: `b7e3999ea15d8d2b`;
- System: `430713d76487fc25`;
- Diagnostics: `3be1346eb2b8c1e5`.

Verification:

- Local Agent `20260920-clay-phase2-v2`: dedicated display/Clay suites PASS, full `make test-host` PASS, production ESP-IDF `make build` PASS, `git diff --check` PASS;
- firmware binary remained `0x4af90`, with 71% of the smallest app partition free;
- Local Agent `20260920-clay-phase2-goldens-v1`: pinned golden hashes and full dedicated display host gate PASS on exact `main` `33e20713b605d93e97fadd7b2c1af8748b3bf897`.

No physical-display claim is made by Phase 2.

### Phase 3 — connect Clay renderer to existing framebuffer/service — NEXT

Replace only the current text-layout layer behind the existing observer transaction.

Acceptance:

- existing async worker remains the slow-work owner;
- `confirmRendered`/success semantics are preserved;
- failure/retry does not advance observer state falsely;
- no control-loop blocking regression;
- full refresh still works before partial-region optimization is enabled.

Implementation rule for this phase: reuse the Phase 2 semantic Clay layout and the existing `DisplayRuntimeFrame`/async transaction. Do not introduce a second navigation owner, a second framebuffer owner or direct Clay-to-SSD1680 hardware ownership.

### Phase 4 — real dirty-region partial transfer

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
- measured transferred bytes/region are observable in diagnostics.

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
4. ESP-IDF build/static analysis;
5. GitHub CI;
6. physical board qualification only for hardware claims.

## Starting a future session

Before implementation:

1. read `../AGENTS.md`, `README.md`, `CURRENT_STATUS.md`, `ARCHITECTURE.md`, this file and `vendor/litegraph_epd_port/README.md`;
2. fetch fresh `main` and verify Local Agent is idle before direct writes;
3. confirm the vendor source pins have not drifted;
4. continue from the first incomplete phase above; currently Phase 3;
5. preserve the shared `growbox_display_model` boundary and existing `DisplayNavigation` ownership;
6. do not reopen SSD1680 pin mapping, rotation, async ownership or SCD41 recovery without new evidence.
