# BloomML — IPO v2 activation transaction checkpoint (2026-11-07)

## Provenance
- Starting main HEAD: `472face97b21aa4e3f838a424d8245b3dee45b17`.
- Code commit: `317bc068abb78b42cc5e10b4568f71ef6a7b30b3`.
- Corrected Makefile commit and exact source qualification SHA: `977c48d221cd310be70f39653a33d8eb4abd5ebd`.
- Execution: Mac Local Agent `la-bloomml-ipo-v2-transaction-tests-20261107-v2` on `agent-control`; state returned `idle`. No new source branch, Codex, Work or GitHub Actions.

## Delivered
1. New single stateful `IpoProfileActivationTransaction`: typed preview token (CRC32 + base generation), revalidation, generation conflict check, explicit safety gate, two-slot NVS commit/readback, common runtime schedule apply and confirmed active RAM/NVS readback. No direct output writes.
2. Production `ClosedIpoActivationGate` refuses all apply; activation owner instantiated on real-input to expose truth. No call path from UART/browser to activate. Rule keeps authority; independent TP357 LampSafety, `OutputSupervisor` and RF433 untouched.
3. `IpoProfileNvsJournal::rollbackTo` for prior good generation or empty namespace; failure to prove RAM+NVS restoration quarantines outputs via the gate interface. NVS CRC32 is integrity, not authentication.
4. Console `ipo_health`, `ipo_readback`, `ipo_store`, `ipo_profile` use dynamic active-vs-stored origin; new `ipo_activation` diagnostic shows last result/failure. On boot active Mint regardless NVS content.
5. Dedicated `ipo_activation_tests`, targeted `make test-ipo` and repaired Makefile regex (Make requires `$$`).

## Targeted tests — actual Mac evidence
- `make test-ipo`: `ipo_profile_tests` PASS, `ipo_activation_tests` PASS; **2/2 CTest**, 0 failed, 0.56 s total.
- `pnpm --dir web exec vitest run src/domain/ipo-board-readiness.test.ts`: **9/9** PASS, 1 file (264 ms wall reported by Vitest). Local locked dependencies restored from cache; no unrelated web build.
- `git diff --check`: PASS.
- Tests cover closed gate, valid commit+apply+readback, no auto apply after reboot, mismatched token/generation, fail after NVS commit and after RAM apply, simulated switch-write fault, CRC mismatch, rollback failure → quarantine, restore prior stored generation. Host fake store/JSON decoder; the real ESP-IDF NVS adapter was **not** executed.
- First trial found malformed `make test-ipo` CTest regex due Make escaping; final runner fixed and retested.

## Safety truth
**IPO v2 activation status: CLOSED / NOT ACTIVE ON DEVICE.** Source includes a conditional transaction but hardware gate rejects it. `stored` is not `active`; no source is authorized to issue physical output commands on behalf of v2. This sprint did not flash firmware, interrogate devices or verify RF physical acknowledgement. Existing physical operator setting remains Mint and Rule-authoritative.

## Remaining blockers
1. New real-input CrowPanel ESP-IDF build and exact-SHA on-device qualification; no inference from host test success.
2. Real `IpoProfileNvsJournal` power-loss/reboot/CRC/fallback behavior and RAM/stack evidence with actual NVS API, not fake journal.
3. TP357/SCD41 stale/invalid sequences, Rule+LampSafety interlocks, independent emergency fan behavior with outputs fake-locked.
4. Real OutputSupervisor-owned critical-section/quarantine gate, safety sign-off and operator authorization before physical apply.
5. Authenticated preview/activation/readback endpoint and live operator panel; no invented "applied" based on browser export or stored CRC.

Next: `docs/NEXT_CHAT.md`. Do not reopen B5/source foundations or past ML research.
