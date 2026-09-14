# Growbox browser configurator

React + TypeScript + Vite frontend for the Growbox ML Controller project.

Public surfaces:

- `/` — schema-driven hardware / JSON configurator
- `/chamber-3d` — interactive React Three Fiber growbox chamber configurator

Live deployment:

- https://michalmatu.github.io/growbox-ml-controller/
- https://michalmatu.github.io/growbox-ml-controller/chamber-3d

## Contract boundary

The browser uses its own explicit bundled contract snapshot:

`schema/environment-controller.v5.json`

That contract is **schema v5: up to 9 pot slots, 228 model features and 25 outputs**.

The repository-root `schemas/environment-controller.json` remains the firmware/controller v4 contract. Migrating firmware from v4 to v5 is separate architecture work and must be deliberate.

`src/domain/schema.ts` validates the browser-side schema version and dimensions at startup so an accidental contract mismatch fails visibly.

## Stack

- React 19
- TypeScript
- Vite
- Three.js / React Three Fiber / drei
- Tailwind CSS / shadcn UI / Radix
- Vitest / ESLint

## Development

Requires Node.js 22 and pnpm 11.10.0.

```bash
corepack enable
corepack prepare pnpm@11.10.0 --activate
pnpm install --frozen-lockfile
pnpm dev
```

From the repository root:

```bash
pnpm --dir web install --frozen-lockfile
pnpm --dir web dev
```

## Quality gate

```bash
pnpm --dir web typecheck
pnpm --dir web lint
pnpm --dir web test
pnpm --dir web build
```

Frontend checks remain separate from firmware/controller checks so one surface cannot silently redefine another contract.

## Routing and GitHub Pages

Vite uses the repository base path `/growbox-ml-controller/`. The 3D route is lazy-loaded so Three.js does not need to load for the JSON configurator.

## 3D chamber

The chamber view contains parametric geometry for the enclosure, pots, lights and fans. It helps visualize hardware configuration and dimensions; it is not CFD or a plant-growth simulation.

The scientific simulator/twin under `tools/ml/twin/` is a separate Python/PyVista engineering tool.

For current repository status and branch policy see [`docs/CURRENT_STATUS.md`](../docs/CURRENT_STATUS.md) and [`docs/HISTORY.md`](../docs/HISTORY.md).
