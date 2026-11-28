# BloomML M05 — autonomous multiparameter Rule

Status: CLOSED_SCOPED — software decision qualification only.
Baseline: ceb234ace7050abb35964cef467266681e29b574
Qualified code: 2a3555a77308f55831fe85b74b2ae2fc5898d32a
Qualified code patch SHA-256: 26bdc2624c287c33435610253d2ed9720616bf338b2cc762f0783ce82d14882c

## Scope
- Existing deterministic Rule remains authoritative. Optional independent day/night CO2 targets travel through the runtime schedule, strict JSON, web and profile wire v2; wire v1 remains readable with CO2 disabled. Mint defaults unchanged.
- Above-target CO2 may request ventilation below the existing high-CO2 protection threshold. Dilution is conditional because outdoor CO2 is not measured.
- Temperature and moisture ventilation opportunities use measured gradients and absolute humidity, not RH alone or a physical chamber forecast.
- Scheduled light alone is not measured heat; near-target anticipation uses an observed rising temperature trend. No inferred wattage, calibrated thermal model or ML/MPC authority.
- Ventilation intent takes precedence over enrichment intent; direct opposing actuators are arbitrated. The existing OutputSupervisor, independent LampSafety and protected activation gate are unchanged. Intent-level exclusion is not physical actuator interlock evidence; no physical CO2 dosing or concurrent actuator safety is qualified.
- Per-goal diagnostics distinguish unmet targets, unusable measurements, invalid targets, missing equipment, absent usable routes, constraints and conditional ventilation. These are decision-side indications, not evidence of physical achievement or guaranteed reachability.

## Evidence
Local Agent isolated clone, Mac:
- Stage 1 4962797: CO2 target ventilation, host and guards PASS.
- Stage 2 v1 failed an exact edit anchor; v2 9ec57ff: profile/runtime/web CO2 and legacy wire, host/web/build PASS.
- Stage 3 c781657: directional ventilation, Python/C++ parity and host/guards PASS.
- Stage 4 3986d0e: lamp trend and conflict handling, Python/C++ parity and host/guards PASS.
- Stage 5 32ef51b: per-goal diagnostics, host/guards/CrowPanel PASS.
- Final exact-code qualification: 2560 deterministic decision cases (64 equipment subsets x 10 scenarios x 2 humidity modes x 2 light states), host 62/62 PASS, Python parity/runtime 13 PASS, architecture 3/3, web 117/117 PASS (typecheck/lint/build through gate), CrowPanel build PASS. growbox_ml_controller.bin binary size 0xd3e10 bytes. Smallest app partition is 0x400000 bytes. 0x32c1f0 bytes (79%) free.
- Output subtree and activation transaction unchanged from M04; no new transport, no new control owner, no hardware operation. Native timing/stack margin not remeasured.

H0 historical PASS, H1 PARTIAL, H2/H3 NOT ADMITTED; ClosedIpoActivationGate remains closed. No physical apply, flash, native NVS, real Shelly or CO2 hardware qualification. Software decision stability is not long-duration physical stability. M06 requires coordinator authorization.
