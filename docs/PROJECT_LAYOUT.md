# Project layout

Use `docs/README.md` for documentation navigation and `CURRENT_STATUS.md` for active work.

```text
.
├── README.md
├── AGENTS.md
├── Makefile
├── CMakeLists.txt
├── config/                     # board/runtime/IDF profiles
├── profiles/                   # example user/controller profiles
├── schemas/                    # production climate-v6 + research contracts
├── docs/                       # current docs, research references, compact history
├── reports/                    # committed ML/model evaluation artifacts
├── tools/                      # host, ML, panel and sandbox tooling
├── scripts/                    # quality/config/runtime guards and helpers
├── examples/
├── third_party/
├── vendor/
│   └── litegraph_epd_port/     # read-only Clay/EPD donor snapshot; not built
├── web/                        # browser configurator/chamber UI
├── lib/
│   ├── environment_control/    # portable climate-v6 controller core
│   ├── growbox_display_model/  # neutral C++17 semantic display page DTO
│   └── growbox_clay_ui/        # isolated C++20 Clay host/layout component
├── components/                 # ESP-IDF components
├── src/
│   ├── main.cpp                # app-mode dispatcher
│   ├── legacy/                 # explicit legacy mode only
│   └── climate/
│       ├── application/
│       ├── display/            # presenter/render seam/raster/SSD1680/service
│       ├── input/
│       ├── native/
│       ├── output/
│       ├── rf433/
│       ├── runtime/
│       ├── storage/
│       └── telemetry/
├── test/                       # portable/host C++ tests
└── tests/                      # Python/tooling tests
```

## Runtime boundary map

- `lib/environment_control/src/climate/ClimatePolicy.*`: stateless Rule/arbitration/safety policy;
- `lib/environment_control/src/climate/ClimateRuntimeController.*`: stateful trends/ML/execution reconciliation;
- `ClimateV6RealInputRuntime.cpp`: bootstrap only;
- `runtime/core/RealInputRuntimeComposition.*`: ownership/lifetime wiring;
- `runtime/core/RealInputRuntimeCoordinator.*`: cycle orchestration;
- `runtime/core/RuntimeOutputTransport.*`: transport truth boundary;
- `runtime/telemetry/*`: telemetry;
- `output/*`: configured-output ownership/policy/execution;
- `rf433/*`: policy-free RF transport;
- `display/*`: observer-only e-ink presentation/render/service.

Do not move climate policy into transport, direct configured-output writes into runtime/console code, or control ownership into display/UI code.

## Configuration boundary

Board/runtime defaults live under `config/` and are resolved by CMake. Production C++ consumes generated typed `RuntimeBuildConfig.h`; do not add duplicate fallback tables in source files.

## Clay/display-port placement

The isolated Phase 1 Clay implementation lives in `lib/growbox_clay_ui/` and remains separate from the normal C++17 production application boundary. Its public API must use growbox-owned Clay-free structs; `Clay_*` types stay private to the component.

The shared semantic page contract now lives in `lib/growbox_display_model/`. It is deliberately header-only and C++17-compatible, and currently contains only `DisplayLine` / `DisplayPageModel`. It must not acquire navigation, snapshot derivation, refresh policy, rendering, hardware or control responsibilities.

`vendor/litegraph_epd_port/` remains immutable/reference-only and excluded from builds.

Phase 2 should consume the shared semantic model rather than depending from `lib/growbox_clay_ui/` back into `src/climate/display/`. If another dependency cycle appears, extract only the smallest stable shared data contract instead of moving runtime ownership into the Clay component.

## Where new work belongs

| Work | Location |
| --- | --- |
| Portable climate behavior | `lib/environment_control/src/climate/` |
| Hardware/runtime orchestration | `src/climate/runtime/`, `src/climate/input/`, `src/climate/native/` |
| Output ownership/policy | `src/climate/output/` |
| RF433 transport/protocol | `src/climate/rf433/` |
| Existing display presenter/backend/service | `src/climate/display/` |
| Shared semantic display page DTO | `lib/growbox_display_model/` |
| Clay layout/host simulator | `lib/growbox_clay_ui/` |
| Clay donor/reference material | `vendor/litegraph_epd_port/` |
| Runtime/board configuration | `config/` |
| Host analysis / ML / sandbox | `tools/` |
| Frontend/chamber UX | `web/` |
| Contracts | `schemas/` |
| Current docs/history/research refs | `docs/` |

## Growth rule

New independent responsibilities should get a focused module under the owning subsystem instead of being appended to a central coordinator/controller for convenience. See `AGENTS.md` for the architecture gate and current oversized-file watchlist.
