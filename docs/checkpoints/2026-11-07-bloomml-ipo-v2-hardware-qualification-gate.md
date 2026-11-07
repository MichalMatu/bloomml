# BloomML — IPO v2: CrowPanel build + production NVS adapter fault matrix
Date: 2026-11-07

## Provenance
- Starting main: `1a1b3de8dada926a00f37be158ef99cd1393b860`.
- Production source exact-SHA ESP-IDF CrowPanel build: `1a1b3de8`; Local Agent `la-bloomml-ipo-v2-crowpanel-build-20261107-v1`, terminal done, exit 0.
- Test/CMake/Makefile source commit `347bcca2901cd4e5d4b52bf27301cad27411ce2a`; Makefile CTest escaping fix `45f352764d819fcdeac939bdd11a9e82a76a0ea3`.
- Subsequent changes after `1a1b3de8` are test and Makefile only, not runtime source. No Codex, Work, GitHub Actions, new source branches, force push, serial port probe or flash.

## Delivered and verified
1. `make build-crowpanel`: real-input CrowPanel N8R8 + e-ink ESP-IDF compile, link and partition checks **PASS**. Binary `0xd1e00` bytes, 80% of 4 MiB app partition free. This is a source build, not a hardware test.
2. Added `test/test_ipo_nvs_journal/nvs.h` and `test_main.cpp`. The production `IpoProfileNvsJournal.cpp` is compiled unmodified against a transactional mock of the ESP-IDF NVS API, with separate volatile handles and committed pages. Covers correct slot/CRC readback; power cut before/after active-marker flip; write failure after actual persistence; corrupt active slot and valid previous backup; corrupt both slots; rollback to empty/prior generation; generation conflict; preservation of distinct output-persistence namespace.
3. `test/host/CMakeLists.txt` and `make test-ipo` include the new NVS target. Initial combined invocation exposed a Makefile dollar-escape error; corrected and verified.

## Mac Local Agent evidence
| Task | SHA | Result |
| --- | --- | --- |
| `la-bloomml-ipo-v2-crowpanel-build-20261107-v1` | `1a1b3de8` | ESP-IDF real-input CrowPanel build **PASS**; no flash |
| `la-bloomml-ipo-v2-nvs-fault-matrix-20261107-v1` | `347bcca` | Production NVS adapter + mock NVS **1/1 PASS**; LampSafety, BLE freshness/Rule, output lifecycle **3/3 PASS**; diff check PASS |
| `la-bloomml-ipo-v2-exact-source-final-gates-20261107-v1` | `347bcca` | **FAIL** due to Makefile CTest recipe escaping; fixed afterward |
| `la-bloomml-ipo-v2-exact-source-final-gates-20261107-v2` | `45f3527` | `make test-ipo` **3/3 PASS**, clean worktree/diff check PASS, authorized port **ABSENT** |

Earlier IPO B5 Vitest 9/9 remains accepted, not repeated. The NVS test executes the actual journal implementation but **mocks the NVS API and does not simulate real ESP-IDF flash, actual power loss or wear**.

## Actual device and safety
- Authorized Mac serial node `/dev/cu.usbserial-120` was **absent**. No serial open, reset, flash, RF command or physical test occurred. Forbidden ports untouched.
- CMake default `GROWBOX_STAGE28_REAL_OUTPUTS_ENABLED=0` remains in `config/runtime/GrowboxRuntimeConfig.cmake`.
- Production `ClosedIpoActivationGate` still denies all active v2 apply. No active browser/UART activation endpoint. Boot runs compiled Mint even when NVS contains an operator record. `stored`, RAM `active`, transport and physical RF acknowledgement must remain distinct.
- Rule remains authority, ML/MPC shadow, OutputSupervisor the only normal output owner, TP357 independent lamp thermal interlock unchanged: trip >=28 C; recovery <=26 C for 10 minutes.

## Not yet qualified
1. Actual authorized CrowPanel hardware with isolated/fake-locked outputs, NVS flash fault/reboot/power-cut evidence, PSRAM/stack/watchdog measurements.
2. Real TP357/SCD41 missing/invalid/stale matrix and physical emergency fan/thermal supervision under safe conditions.
3. OutputSupervisor-owned quiesce/emergency-lock with explicit operator and safety sign-off; no uncoordinated apply.
4. Authenticated local device preview/activate/status/rollback channel and existing operator frontend integration. The older `tools/panel` serial bridge is a research/demo protocol, not automatically an authenticated production API.

Next: `docs/NEXT_CHAT.md`. Do not claim firmware deployment, physical NVS acceptance or active IPO v2 before independent evidence.
