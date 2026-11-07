# BloomML — continuation after IPO Foundation v2 (2026-11-07)

## Current baseline and work policy

- Source: `main` only. `agent-control` for Mac Local Agent; `gh-pages` publishing. Never Codex, Work, GitHub Actions.
- Read `AGENTS.md`, `docs/CONFIGURATION_FLOW.md`, `docs/CURRENT_STATUS.md`, `docs/ARCHITECTURE.md` and this file; fetch fresh `main` and `agent-control:.agent/status/daemon.json` before any task.
- Operator explicitly requested avoiding repeat tests and heavy builds. **No new test/build qualification for the IPO v2 source has been performed**; do not call it verified.

## What was closed

- `schemas/device-profile.v2.schema.json` + `profiles/crowpanel-ipo.v2.json`: open bounded IPO graph profile, not fixed 3-socket product limitation.
- `web/src/domain/ipo-catalog.ts`, `ipo-profile.ts`: driver capabilities, semantic signal sources/fallback order, physical output registry, process→effect→output bindings, draft validation, v1 migration, symbolic MAX intents.
- Browser `/` is the new single main v2 editor; `/lab/profile-v1` preserves v1, `/lab/v5` preserves research v5; `/chamber-3d` independent geometry.
- `lib/ipo_profile/src/IpoGraph.h`: unlinked C++17 bounded DTO, structural graph validator, symbolic arbiter for future firmware integration.
- Canonical matrix/limitations: `docs/CONFIGURATION_FLOW.md`. Checkpoint: `docs/checkpoints/2026-11-07-bloomml-ipo-foundation-v2.md`.

## Next sprint: connect v2 to real runtime WITHOUT rewriting existing safety

1. Decide supported physical driver set for this CrowPanel and parse v2 into typed bounded DTO, fail closed on unknown/unsupported driver/locator. New drivers require explicit adapter, not generic arbitrary code.
2. Attach sensor roles with freshness/valid/fallback, preserving TP357 current role and acknowledging SCD41 currently CO₂-only.
3. Apply a coherent, validated day/night target/schedule snapshot to both climate Rule and lamp schedule.
4. Map compatible logical output intents through existing `OutputSupervisor` and binary policies only, preserving thermal safety and transport attempt-vs-physical observed distinction. ML remains shadow.
5. Implement dedicated profile NVS version/CRC/hash, active/pending/known-good, atomic commit/readback/recovery and revert to Mint on invalid state. Output state persistence remains separate.
6. Expose actual status + active profile hash through one safe local API and existing panel/e-ink observer, not a second control owner.
7. Separate explicit targeted qualification when requested; don't re-run full historical batteries by default.

The v2 browser export is **draft_not_applied**; no real device API/apply/readback exists yet, so do not claim the board runs the v2 graph. MPC research 19/22 prior hard-case FAIL and no empirical growbox calibration remain open; no active hardware authority.
