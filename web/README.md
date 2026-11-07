# BloomML — browser setup, research editor and 3D

One simple operator entry point, preserving research tools:

- `/` — **device profile v1 draft**: sensors, the 3 fixed physical RF433 output roles, day/night T and RH/VPD targets, schedule, validation, JSON import/export.
- `/lab/v5` — original **research** hardware editor (228 features, 25 outputs); not a firmware config.
- `/chamber-3d` — independent geometry preview; not the physical environment simulator and not synchronized with the device profile.

Published base: https://michalmatu.github.io/bloomml/ . Routes use the `/bloomml/` Vite base path.

## One place to understand it

**[CONFIGURATION_FLOW.md](../docs/CONFIGURATION_FLOW.md)** contains the real sensor→target→output matrix, scope and integration order.

- Operator JSON contract: `../schemas/device-profile.v1.schema.json`.
- Current compiled-in Mint defaults mirrored in the example: `../profiles/crowpanel-mint.v1.json`.
- Strict draft parsing/validation: `src/domain/device-profile.ts`.
- Offline draft page: `src/pages/setup-page.tsx`.
- Research editor remains on v5: `schema/environment-controller.v5.json`.
- Firmware's `../schemas/environment-controller.v6.json` is an **ML inference** contract (44 features, 6 decisions), not the operator profile.
- The v4 research schema and the Python physics/MPC lab are separate.

**Important:** `deployment_status=draft_not_applied`. No browser-to-ESP32 connection, profile NVS, live sensor status, arbitrary RF remapping or device readback exists in this page. Declared `installed` is not a sensor probe. Production still runs the hard-coded Mint schedule. The old v5 JSON is not converted or uploaded to firmware.

## Development

React 19 + TypeScript, Vite, Tailwind/Radix, optional Three/R3F. Node 22 and pnpm 11.10.0.

```sh
pnpm --dir web install --frozen-lockfile
pnpm --dir web dev
```

Existing typecheck/lint/test/build commands remain available, but **no tests or heavy builds were run during this configuration draft sprint at the operator's request**; deployment remains unverified.
