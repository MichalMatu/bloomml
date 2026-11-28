# BloomML M04 — wymienne i konfigurowalne wyjścia IPO

Status: CLOSED_SCOPED — software only; no physical activation or qualification.
Baseline: f091bdd1356fe73b53b003ad77069f612811e9b6
Qualified code: d17c781d7e326980628176a83e6117af8766f2a2

## Scope and boundaries

- Portable IpoOutputPlan validates up to seven MVP roles: light, exhaust, heater, cooler, humidifier, dehumidifier, CO2 doser. All 128 subsets are tested. Zero outputs is a valid draft and explicitly refused by the executable supervisor; no phantom output.
- ConfiguredOutputEquipment projects validated device/role/channel/dwell to installed adapter routes, endpoint policy, semantic bindings and ClimateCapabilities. ConfiguredOutputPolicies owns bounded per-endpoint binary dwell state; transport only translates stable endpoint IDs to adapter-local endpoint IDs and returns transport evidence. Missing adapters and conflicts fail closed. Process and Rule have no device or protocol dependency.
- Mint production composition uses the compiled reference graph, existing RF433 registry and legacy endpoint IDs 1/2/3. The reference lifecycle order, timing, and capability flags are checked against the old defaults. Schedule and independent LampSafety resolve optional lamp/exhaust endpoints; trip/recovery timing remains unchanged. The existing OutputSupervisor is still the only normal configured-output execution owner. Maintenance remains the explicit Mint RF-only exception.
- Host coverage includes all subsets, seven simultaneous output intents (not Rule arbitration), 1–7 endpoint boot lifecycle, unavailable adapter, replacement adapter, duplicate routes, optional safety/schedule endpoints, dwell and command vs physical truth. The dual-port Shelly switch/meter draft is tested using a fake implementing DeviceElectricalSource and OutputTransport; power/energy samples never imply command ACK or physical state.
- Persistence GOP1 envelope version 2 has 290 bytes; version 1 (134 bytes) is decoded with CRC, exact length and original role range. Legacy command state is restored without inventing physical observations; migration writes v2 on next successful synchronization and retries failed writes. Incompatible role-to-endpoint mapping is rejected before restoring state. Stable installed endpoint identity is a precondition: physical-device replacement across boot is not qualified; no generic device identity migration or live reconfiguration is authorized.
- Production UI/profile apply remains draft-only. Shelly switch/meter is proposed in the graph and web catalog; native locator decoder and board admission remain restricted to compiled Mint hardware. No real Shelly, GPIO, Modbus or PWM driver added. Non-MVP roles, continuous output execution, native NVS/hardware tests, timing/stack measurements, dynamic activation and M05 are outside scope.
- All new state is bounded by seven endpoints; no new tasks, queues, per-cycle allocation or I/O owner. Display and telemetry remain observers; existing fixed Mint display and maintenance interfaces remain reference-specific.

## Evidence

Local Agent (Mac), isolated clone, no hardware, Codex, Work or GitHub Actions:
- M04.1: b9078cced5016eb6f231c1dfb69e145039006f0a — all 128 output subsets, IPO 3/3, guards 3/3.
- M04.2: f5e3412f01dc30d03598dfc719ca41263740f88c — capacity and v1 decode, full host and guards PASS.
- M04.3A: 46723cfeaa71a4dcb2924bfaac8bf88638df9b51 — adapter projection/replace/missing/conflict, targeted host and guards PASS.
- M04.3B: b3571ae4633b2106c9e06fbb900dfbfda551e125 — production Mint equipment composition, all seven endpoints and dwell, host and CrowPanel PASS.
- M04.4: 0faf1972be45fe9d48718a8db637b6b9566a2145 — optional safety, dual ports, stale mapping rejection, host/web 115/115/CrowPanel PASS.
- M04.5: d17c781d7e326980628176a83e6117af8766f2a2 — final lifecycle, Mint parity, migration retry, and web dual-port draft qualification.
- Publication requalification at exact code SHA: make test-host 53/53 + 9/9 PASS; architecture guards 3/3 PASS; pnpm gate (TypeScript, lint, Vitest 116/116, web build) PASS; ESP-IDF CrowPanel build PASS (binary 0xd3560 bytes). Nonblocking web chunk-size warning. No native resource timing or physical validation claimed.
- Architecture review: portable graph validation and application semantic ports remain independent of transports; composition wires owners; resolver/lifecycle keep arbitration and safety; persistence stores only command truth; no UI/output authority added.

Control-plane results:
- .agent/results/la-bloomml-m04-output-plan-20261128-v1.json
- .agent/results/la-bloomml-m04-supervisor-capacity-20261128-v1.json
- .agent/results/la-bloomml-m04-equipment-adapters-20261128-v1.json
- .agent/results/la-bloomml-m04-runtime-equipment-20261128-v1.json
- .agent/results/la-bloomml-m04-optional-safety-dual-device-20261128-v1.json
- .agent/results/la-bloomml-m04-final-regression-20261128-v1.json
- .agent/results/la-bloomml-m04-publish-20261128-v1.json

H0 historical PASS, H1 PARTIAL, H2/H3 NOT ADMITTED. ClosedIpoActivationGate retained. M05 not started.

Qualified code patch SHA-256: f705eb520371de56e54e6fb39a7106ab44390ef173534afc8adc9bb745657a39
