# Data contracts

This repository currently contains two distinct contract generations. They serve different purposes and must not be treated as one source of truth.

## 1. Production climate-v6 contract

Source: `schemas/environment-controller.v6.json`.

This is the contract used by the current portable climate-v6 controller/runtime architecture. It has the bounded climate feature/output surface described by `docs/MODEL_PIPELINE.md` and generated `ClimateContract.h`.

Production ownership rules come from `ARCHITECTURE.md`, not from the older v4 simulator schema.

## 2. Broad v4 growbox simulator/tooling contract

Source: `schemas/environment-controller.json` (schema version 4).

This older contract remains active inside parts of `tools/ml` and the simulator/config-matrix research tooling. It models one growbox air volume with up to four pots, 128 encoded features and 15 normalized actuator outputs.

It remains in the repository because working tools depend on it. It is **not** the production real-input runtime contract.

Research rules:

- field names/order/ranges come from the v4 schema;
- sensor validity masks control imputation;
- actuator `available=false` forces zero capability/safe output;
- simulator physics lives under `tools/ml`, not in the JSON contract;
- schema/hash mismatch must be rejected by the matching model/runtime tooling.

Regeneration for that toolchain:

```bash
python tools/schema/generate_environment_schema.py
python tools/schema/generate_environment_schema.py --check
```

Related research docs:

- `CONFIG_MATRIX.md` / `.csv`;
- `simulator/IO_INVENTORY.md`;
- `simulator/README.md`.

## Change rule

Before changing either schema, identify which generation the task targets. Do not migrate production climate-v6 behavior by editing the v4 schema, and do not silently rewrite the v4 simulator contract to match production.

A deliberate convergence/migration of the two contracts would be a separate project requiring tool, generated-code, dataset/model and runtime compatibility planning.
