# BloomML web configurator — unified IPO setup

Public paths:
- `/`: **DeviceProfile v2** single IPO editor: devices/drivers, semantic input signals and primary/backup, logical outputs, typed processes, links, targets and JSON import/export.
- `/lab/profile-v1`: preserved original fixed Mint DeviceProfile v1 editor; validated migration v1 → v2 available on root.
- `/lab/v5`: historical 228-feature research JSON editor.
- `/chamber-3d`: separate chamber geometric preview, **not** the climate simulator.

Contract: `../schemas/device-profile.v2.schema.json`. Example: `../profiles/crowpanel-ipo.v2.json`.
Domain: `src/domain/ipo-catalog.ts` + `ipo-profile.ts`. Canonical source-of-truth I/O map and limitations: [`docs/CONFIGURATION_FLOW.md`](../docs/CONFIGURATION_FLOW.md).

The UI is **draft-only**: no active-device profile write, live status, NVS commit, or firmware driver registry. New physical endpoints/drivers in the form require a supported ESP32 runtime adapter and separate hardware qualification. Rule and OutputSupervisor remain the authority. SCD41 currently supplies CO₂ only to firmware and is not a functioning backup for TP357 T/RH. ML Shadow is passive.

Research v4/v5 and production inference climate-v6 are separate contracts; never import/export them as DeviceProfile v2.

Build tools: Node.js 22, pnpm 11.10.0, React 19 + TypeScript, Vite, Tailwind/Radix, optional R3F. Existing commands `pnpm --dir web dev`, `typecheck`, `lint`, `test`, `build`; **none were executed in this sprint by operator request**, so this new source has no new executable quality evidence.

## B5 current-board readiness preview

`src/domain/ipo-board-readiness.ts` cross-checks a valid open IPO v2 draft against the **reference-only** CrowPanel Mint mapping (supported driver locators, CO₂, primary sources, all protected routes, fixed RF433 sockets and dwell). `/` shows reasons for incompatibility; **apply is always disabled**. Profiles with future drivers remain valid *drafts*. Compact UTF-8 JSON export is capped for the future firmware parser at 32 KiB; per-device labels must fit the C++ parser's 80-byte UTF-8 limit to be board-compatible. New checks: `src/domain/ipo-board-readiness.test.ts` (authored, **not run**). There is still no live device API or authoritative hardware status in the browser.
