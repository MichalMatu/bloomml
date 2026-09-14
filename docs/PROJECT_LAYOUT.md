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
├── lib/environment_control/    # portable climate-v6 controller core
├── components/                 # ESP-IDF components
├── src/
│   ├── main.cpp                # app-mode dispatcher
│   ├── legacy/                 # explicit legacy mode only
│   └── climate/
│       ├── application/
│       ├── display/            # current presenter/raster/SSD1680/service
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

## Display-port placement

The planned Clay integration should not be implemented inside `vendor/`.

Preferred destination is a dedicated `components/growbox_clay_ui/` C++20 component, while the existing `src` application component remains C++17. Shared public structs must stay Clay-free. If a dependency cycle appears, extract the minimal shared display model into a small C++17 component rather than exposing Clay internals.

`vendor/litegraph_epd_port/` remains immutable/reference-only and excluded from builds.

## Where new work belongs

| Work | Location |
| --- | --- |
| Portable climate behavior | `lib/environment_control/src/climate/` |
| Hardware/runtime orchestration | `src/climate/runtime/`, `src/climate/input/`, `src/climate/native/` |
| Output ownership/policy | `src/climate/output/` |
| RF433 transport/protocol | `src/climate/rf433/` |
| Existing display backend/service | `src/climate/display/` |
| Future Clay layout engine | `components/growbox_clay_ui/` |
| Clay donor/reference material | `vendor/litegraph_epd_port/` |
| Runtime/board configuration | `config/` |
| Host analysis / ML / sandbox | `tools/` |
| Frontend/chamber UX | `web/` |
| Contracts | `schemas/` |
| Current docs/history/research refs | `docs/` |
