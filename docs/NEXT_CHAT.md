# BloomML — IPO Runtime Bridge continuation (2026-11-07)

## Repo / execution
One development branch: `main`; `agent-control` Local Agent, `gh-pages` publishing. No Codex, Work, GitHub Actions. Read `AGENTS.md`, `docs/CONFIGURATION_FLOW.md`, `docs/ARCHITECTURE.md`, `docs/CURRENT_STATUS.md` and checkpoint `docs/checkpoints/2026-11-07-bloomml-ipo-runtime-bridge.md`; fetch current HEAD, never assume an old SHA.

## Implemented, do not repeat
- `DeviceProfile v2` open bounded graph (devices, semantic input roles/fallback priorities, processes/effects, outputs, links) in `schemas/device-profile.v2.schema.json`, `web/src/domain/ipo-*.ts`, browser `/`, legacy `/lab/profile-v1` and `/lab/v5`.
- New firmware-facing shared **in-RAM** `RuntimeScheduleProfileOwner` with strictly validated Mint-compatible day/night targets; real Rule snapshot and lamp schedule now read the same owner through `RuntimeAdapters` and `RealInputRuntimeCoordinator`; existing Mint factory maintained.
- `status` console exposes profile generation and day/night values with explicit `persisted=0`, `profile_v2_applied=0`.
- `lib/ipo_profile/src/IpoBoardAdmission.h`: reference CrowPanel graph, current physical driver/mapping admission, **apply_authorized always false**. `IpoSignalSelector.h`: semantic priority/freshness selector, not yet connected to real sensors. Firmware startup logs read-only admission.
- `src/climate/runtime/profile/IpoProfileWire.h` + `IpoProfileNvsJournal.h/.cpp`: **whole typed v2 graph + targets** wire codec (length/magic/version/CRC, cap admission), separate dual-slot NVS journal with readback/backup and generation checks. Source is included in real-input CMake, but **not invoked at runtime**; no profile NVS write/read or apply occurs. OutputSupervisor/Safety/output policy untouched.
- Operator requested **no repeated tests or heavy builds**; none run. New integration is **UNVERIFIED** (no MCU deployment).

## Next autonomous sprint (bounded and closable)
1. Implement strict, bounded v2 JSON → typed graph/profile decoding, reject unknown device drivers, locators, invalid source/channel and unsafe mappings at the firmware seam. Do not allow arbitrary code/RF.
2. **Do not redo the now-authored full-profile binary codec/two-slot NVS journal.** Add its focused corruption/torn-write/generation/back-up qualification when explicitly permitted, then a privileged single-task coordinator: decode/validate → stage NVS → atomic apply → readback/rollback. The journal source is currently dormant, without boot invocation or browser API.
3. Use signal-selector to safely route timestamped sensor readings (TP357 primary, SCD41 CO₂; T/RH backup requires separate evidence). Do not claim all proposed drivers work.
4. Connect any approved process output intent only through `OutputSupervisor`, and add exact-state readback, transport attempt vs independently observed truth.
5. Provide browser profile `preview → validate → apply → readback` only when a real local device API exists. E-ink observer only, Shadow passive.

No v2 profile has been saved to NVS or applied to real board. Do not claim active v2 or successful build/tests. Research MPC 19/22 old hard-case FAIL, no empirical growbox calibration remain open.
