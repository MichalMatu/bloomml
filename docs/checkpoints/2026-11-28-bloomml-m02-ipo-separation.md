# BloomML M02 — IPO implementation ports

Status: CLOSED_SCOPED — software separation only; not physical qualification.
Baseline: f780b2d4a88c2cf72c7160295d8fdea274eb0341
Code commit: 326850099912508eb4026cf4d8bdac82d45e81a6
Reviewed patch SHA-256: 9bbf2e06189fd4e43fa74106d94290ea1a1f91950cf76571e11f2430ef7890f2

## Scope
Composition binds ClimateInputSource, ClimateProcess and ClimateActuatorSink. Independent host substitution tests cover each port, execution feedback/reset, measurement status and a dual-role electrical input/output fake. Rule retains its existing usability predicate. TP357 remains the independently bound LampSafety source. OutputSupervisor retains physical output ownership. Electrical telemetry is a contract seam, not a real Shelly driver, physical ACK or thermal model. Mint bindings and closed activation remain unchanged.

## Evidence
Initial Local Agent M02-v1 failed compilation. Recovered changes were backed up; the operator applied reviewed header/test fixes locally. Operator-provided Mac logs show make test-host PASS: 53/53 main CTest suites plus 9/9 display/Clay tests; all three architecture checks PASS; ESP-IDF 5.5 CrowPanel build complete, binary 0xd1e20, 80% app partition free. Those runs preceded commit, and the audited patch was identified by SHA-256 above; firmware build metadata therefore references the baseline rather than the new commit. No new hardware evidence. Publication task verifies the exact patch hash and reruns the three architecture guards; it does not claim to have independently rerun the operator's build or tests.

H0 historical PASS, H1 PARTIAL, H2/H3 NOT ADMITTED, ClosedIpoActivationGate unchanged. No flash, serial, hardware operation, native NVS or ML/MPC activation. M03 requires separate coordinator authorization.
