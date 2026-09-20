# Changelog

Only milestones that still help understand the current codebase are kept here. Detailed phase-by-phase notes remain in Git history.

## Unreleased

### Clay production display integration — 2026-09-20

- Completed Display UI Port Phase 3: the isolated Clay 0.14 renderer now drives the production four-page CrowPanel layout behind the existing asynchronous display transaction.
- Preserved the native SSD1680 backend as sole owner of the 296x128 / 4736-byte framebuffer and all panel hardware; no second framebuffer, worker, navigation owner or control path was introduced.
- Added one persistent Clay scratch arena allocated in PSRAM by the display service; firmware rendering reuses it instead of allocating per refresh.
- Preserved generation matching, `confirmRendered()` and retry/failure semantics; failed Clay/backend frames are cancelled without falsely advancing observer state.
- Unified host and firmware framebuffer polarity with the production raster convention (`0 = black`, `1 = white`) and re-pinned deterministic four-page hashes without changing black-pixel counts/layout.
- Added `make build-crowpanel` as the canonical CrowPanel N8R8 + Stage27 NimBLE + Stage27C + real-input + e-ink production build and added a dedicated CI job for that path.
- Exact Phase 3 production-path build evidence: `76447b963bb1e3f4d9290bfb26380ac30a0a41ab`, Local Agent `20260920-clay-phase3-crowpanel-build-v4`, binary `0xcc970`, 80% of the 4 MiB app partition free.
- This milestone is software/build evidence only; no new physical CrowPanel qualification is claimed.

### Documentation/vendor preparation — 2026-09-14

- Re-audited live documentation against the current `main` line and removed the stale SCD41-as-active-blocker narrative.
- Added `docs/README.md` as the single documentation entry point.
- Added `docs/DISPLAY_UI_PORT.md` as the live contract for later Clay/menu/button/simulator work.
- Separated production `environment-controller.v6.json` documentation from the older v4 128-feature/15-output simulator/tooling contract.
- Updated architecture/layout/roadmap/I/O docs so future work does not reopen completed display backend or SCD41 recovery work.
- Curated `vendor/litegraph_epd_port/` as read-only reference material pinned to `esp32s3_LiteGraph@5b8c758c365547ddeaab65bbe9f849bdd071695d`.
- Captured Clay renderer, dirty-region, menu/navigation, SDL and host-test references while keeping vendor code excluded from all builds.
- Documented the preferred isolated C++20 Clay component boundary while the production application remains C++17.

### SCD41/e-ink closeout — 2026-09-14

- Final code-bearing closeout: `c720c0a1d6d6d9c80daeb04b2dc69efa53d2c8a4`.
- Preserved `wake_up -> stop_periodic_measurement -> reinit -> start_periodic_measurement`.
- Added one bounded 30-second liveness recovery at most once per MCU boot.
- Retained four-page Environment / Outputs / System / Diagnostics UI and async observer-only display ownership.
- Historical clean-start/e-ink hardware baseline: `a92074b74b055c58c0949c7c38f4896638ebf227`, 90-second run plus 5/5 MCU-reset cycles.

### Repository consolidation — 2026-09-14

- Promoted the active runtime line to canonical `main`.
- Removed superseded phase handoffs, temporary prompts and one-off audit plans from live documentation.
- Kept long-lived branch policy to `main`, `agent-control`, `gh-pages`.
- Consolidation intentionally did not change controller behavior.

### CrowPanel e-ink integration — 2026-09-13

- Added observer-only native SSD1680 e-paper runtime integration.
- Added rotation, async display worker and partial-refresh waveform support.
- Physical display baseline: `01db8228e6d822b5c64359abd8bf2d85341b5919`.

### Release-readiness and structural hardening — 2026-09-11 to 2026-09-12

- Split real-input runtime, service-console and output ownership into narrower production boundaries.
- Centralized runtime/build configuration.
- Preserved Rule authority and `OutputSupervisor` configured-output ownership.
- Structural cleanup: `0a7097a30280ec0f7bb408799c07093761d63e88`.
- Release-readiness: `e03763d019af405087a5fa9c6713a7165d2e623f`; CI #865 / Sandbox Pack #63.

### Climate-v6 / native ESP-IDF platform

- Added Rule / ML shadow / gated active-policy research capability with Rule as production authority.
- Added climate-v6 runtime, trace/replay/counterfactual evaluation and generated C inference.
- Migrated standalone firmware to native ESP-IDF 5.5.4.
- Added real-input adapters, RF433/output execution architecture, diagnostics, host/HIL tests and simulator/twin tooling.

## 0.1.0 — 2026-07-11

- Bootstrapped the schema-driven environment-controller library.
- Added deterministic simulation/training/export pipeline and ESP32-S3 demonstration firmware.
