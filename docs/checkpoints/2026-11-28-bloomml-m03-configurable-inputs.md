# BloomML M03 — wymienne wejścia IPO

Status: CLOSED_SCOPED — software only; physical activation remains closed.
Baseline: 2e816ce6a8b57d47e98028b3f27d6f359c40025e
Qualified code: 4fc585d3155bddb115d3c5e6a52123f791edaf8c

## Scope and limits

- Bounded input observation registry and existing priority selector resolve configured device IDs by installed driver/locator, not fixed Mint numeric IDs. SCD41 T/RH/CO2 and both existing BLE sensors can supply graph-defined environmental roles. Unknown, stale and invalid observations are rejected; unavailable/invalid/stale counts describe sources considered by the selector, not every raw rejected hardware packet.
- Configured DS3231 clock uses the same semantic clock role for the Rule snapshot and lamp schedule, sharing a timestamped result. Native RTC diagnostics remain separate.
- Host tests cover Mint, SCD41-only environmental readings, BLE-only with absent CO2, mixed inputs, priority fallback, missing/invalid/stale readings, ID substitution and clock validity. BLE-only does not synthesize CO2; independent TP357 LampSafety still requires its own hardware reading.
- Optional active_power_w / energy_wh and shelly.meter are draft input capabilities in C++, schema and web catalog; no Shelly transport, output authority, physical reading, or telemetry publication is implemented.
- Production continues to boot the built-in Mint reference graph. Generic source substitution is software-tested, not a deployed alternate profile. Board admission, wire version, protected Rule, LampSafety, OutputSupervisor and ClosedIpoActivationGate remain unchanged. No physical apply, dynamic driver discovery, hardware test, flash, native NVS or ML/MPC activation.

## Evidence

Local Agent, Mac, isolated clone, exact code commit above:
- Stage 1 registry: edda0f2b74a33a887610c9d9785fceea7b3773db; IPO 3/3 and architecture 3/3 PASS.
- Stage 2 environmental adapter: b03d01b6d839394592a2b28079f8d94ca39fe938; IPO 3/3 and architecture 3/3 PASS.
- Stage 3 clock: d77a722e45799a81ea0d7a47adacd62b8040a232; IPO 3/3 and architecture 3/3 PASS.
- Stage 4 v1 failed TypeScript because the Shelly draft device factory lacked a locator. v2 corrected the factory and added a regression. IPO 3/3, guards 3/3, Vitest 115/115, TypeScript, lint and web build PASS. Nonblocking large web chunk warning.
- Stage 5 terminal result done / command exit 0: full make test-host and all three architecture checks PASS; ESP-IDF 5.5 make build-crowpanel PASS, app binary 0xd22c0, 79% partition free. Result output retains the build tail; successful command completion gates earlier tests. Publication rechecks host with a compact summary and verifies the qualified code and unchanged base before a normal push. Compiler SDK warnings do not indicate a failed build.

Control-plane results:
- .agent/results/la-bloomml-m03-input-registry-20261128-v1.json
- .agent/results/la-bloomml-m03-environment-wiring-20261128-v1.json
- .agent/results/la-bloomml-m03-clock-quality-20261128-v1.json
- .agent/results/la-bloomml-m03-electrical-contract-20261128-v2.json
- .agent/results/la-bloomml-m03-final-regression-20261128-v1.json

H0 historical PASS, H1 PARTIAL, H2/H3 NOT ADMITTED. M04 is not started.

Qualified code patch SHA-256: 1c0ea4b9f9c3c1adec9fe2768532bc7159934aacf494c3c945b25f5392882b40
