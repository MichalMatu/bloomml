# Model pipeline

Status: research reference. Production remains Rule-authoritative; this document is not the active product roadmap.

## Production climate-v6 ML contract

The current climate-v6 research-capable core uses 44 ordered inputs and 6 climate outputs with a bounded `44 -> 32 -> 32 -> 6` MLP candidate path.

Outputs:

- heater;
- cooler;
- exhaust fan;
- humidifier;
- dehumidifier;
- CO2 doser.

Effective actuator-state estimates are advanced from the safe/applied semantic action, never from raw ML requests.

## Frozen research decisions

Detailed metrics are in `ML_DECISION_REPORT.md`.

- Sequence Teacher: keep for new climate labels.
- Effective actuator observability: keep.
- Residual ML policy: rejected on available evidence.
- Generic deterministic CO2/exhaust coupling candidate: rejected on available evidence.
- One bounded Sequence-Teacher DAgger iteration improved the prior ML candidate but failed switching/Rule comparison gates; no second iteration and no publication.

Do not restart broad training from the same synthetic assumptions without a new hypothesis or real/calibrated data.

## Production authority

`ClimateRulePolicy` remains authoritative. ML is shadow/research-only until a separate acceptance and hardware qualification explicitly changes that policy.

Safety remains authoritative regardless of policy mode.

## Current useful ML work

After higher-priority product/display work, useful research includes:

- collect deterministic runtime traces;
- replay Rule vs ML shadow on real trajectories;
- calibrate simulator dynamics from hardware evidence;
- improve Python/C++ parity and explainability;
- reconsider model architecture only when new data creates a concrete hypothesis.

## Separate v4 simulator toolchain

Some `tools/ml` modules still use the older `schemas/environment-controller.json` v4 contract with 128 features / 15 outputs. That broad pot/simulator contract is documented in `DATA_CONTRACT.md` and `docs/simulator/` and must not be confused with this production climate-v6 surface.

Closed-loop behavior remains the primary model gate. Offline MAE/F1 alone does not qualify a controller.
