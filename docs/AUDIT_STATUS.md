# BloomML — repository audit and repair ledger

As of 2026-11-07. Scope: `MichalMatu/bloomml`. Source baseline `main@d5044b712b9f7b56278c31f5770d853836089071`. Former research tip `73de11be282d40d03767275cc6a0891e58ed23f8`, merged into `main` by `fadf6614e54264ad9f45ca19ce8699683322bb23`.

## Reconstructed state

- Main: stable production climate-v6 with Rule authority, configured-output safety and CrowPanel observer. Recent main history: the last 7 and 30 days contain the Nov 3/Nov 5 manual-only CI/storage commits; larger 90-day history includes display, architecture, ML decisions and performance tooling.
- Research at its historical tip: 110 commits ahead of the common base with physics R4, Kaggle 2×T4, datasets and MPC. The full research ancestry is merged into `main`, but **none of this is production firmware authority**.
- Local Agent: `agent-control:.agent/status/daemon.json` observed idle with no active task; daemon 4.20.16 on 2026-11-07. No hardware/Mac/Kaggle task launched as part of this audit.
- Archive: Library snapshot `growbox-source-deb68799899450d248b7e300e2bc20374b42ba8a.tar.zst` checksum verified; it predates current main and is NOT an exact current-main checkout. Several target panel files match current main blob SHAs.

## Confirmed issues and actions

| Finding | Action | Verification |
| --- | --- | --- |
| Two separate research/admin interfaces are presented without a single production setup path | `PRODUCT_DIRECTION.md`, revised readmes and roadmap describe explicit ownership; no dangerous schema conversion | Documentation and source-inventory review |
| v4/v5/v6 contracts are conflated in browser text and `web/README.md` | Corrected browser wording, fallback filename, source-of-truth label; clear research-only v5 disclaimer | Offline web snapshot: typecheck, lint, 104 Vitest tests, production build PASS |
| Local serial panel accepted cross-origin POST and unbounded Content-Length | Same-origin / fetch-site check, loopback Host check, JSON-only POST, 256 KiB body cap; regression suite | Local 62 focused pytest checks PASS using pyserial import stub |
| Main README described display Phases 1–3 as current despite 4–6 being integrated | Documentation correction | Source status cross-check |
| Research MPC gate not satisfied (19/22 hard scenarios FAIL, physical fidelity UNKNOWN) | Preserve research isolation; document promotion criteria and next retest | Existing checkpoint only; **not rerun** |
| Physical button and SCD41 qualification open | Preserve as open physical gate | **Not run** |

## Evidence boundaries

The panel suite was run in ChatGPT Linux Sandbox from the checksum-verified archived snapshot with exact-current-main panel server and existing relevant tests (matching Git blob hashes); the sandbox lacked pyserial, so an **import-only stub** was used and no real serial transport was tested. The new HTTP security tests used loopback ThreadingHTTPServer.

The web quality gate ran against the archived offline dependency snapshot and patched v5 UI text: `pnpm gate` (typecheck, ESLint, 104 Vitest tests, build) PASS; bundle-size warning for 3D remains. This is not an exact-current-main clean-room build. No current-main ESP-IDF build, end-to-end browser-to-firmware setup, physical sensor/output test or fresh Kaggle MPC benchmark has been demonstrated here.

No claim of a full repository audit or hardware qualification is made. Do not disable gates or silently convert UNKNOWN/BLOCKED into PASS.

## Remaining audit work (bounded, not repeated)

1. Fresh exact-HEAD source/dependency snapshot; full project layout/dependency/security inspection and all relevant host/web/IDF gates.
2. Local Agent host preflight + authorized SCD41/buttons physical qualification when hardware/operator conditions are available.
3. Device-profile design with explicit v4/v5/v6 isolation; prototype only after contract and acceptance tests.
4. On `main` under isolated `tools/ml/`: confirm merged-head regression, analyze 19/22 MPC failures, then bounded CPU/GPU/Kaggle research. Do not activate unqualified MPC in firmware.

## Checkpoint discipline

This is a scoped repair checkpoint rather than a false "all gates green" declaration. Current source HEAD must be fetched anew before a subsequent change, and final commit SHA is reported by the publisher rather than embedded recursively in this file.

## Consolidation (2026-11-07)

Two-parent merge `fadf6614e54264ad9f45ca19ce8699683322bb23` retains previous product main `5e4db5af736e9c18d69d451496694fa6aff11743` and experimental tip `73de11be282d40d03767275cc6a0891e58ed23f8` as parents. All 72 research-only modified/added paths were copied by exact blob SHA; the three overlapping active docs were reconciled. Existing product code remains unchanged. Research branch was deleted after successful Local Agent ancestry/blob/quality verification (`la-bloomml-main-consolidation-20261107-v2`, done/exit 0); no other work branch remains. Never infer hardware qualification from this merge.

### Exact-main cleanup verification

At main `8ed884a56c91fcd8650796cdb3db7ea4cbf37312`, Local Agent result `agent-control:.agent/results/la-bloomml-main-consolidation-20261107-v2.json` confirms: Git ancestry PASS, 69/75 research-changed paths byte-identical (the remaining six are intentionally reconciled/updated Markdown), production code unchanged, Python research compileall PASS, **37 MPC unittest PASS**, **18 dataset unittest PASS**, clean checkout, and successful remote deletion of `research/simulator-fidelity-kaggle-20261103` via normal `git push --delete`. Remaining branches: `main`, `agent-control`, `gh-pages`. This is scoped verification, **not** firmware or all-tests PASS.

## 2026-11-07 configuration draft sprint

Mapped actual TP357/SCD41/Xiaomi/DS3231 inputs, fixed RF433 sockets and day/night Mint control from source, then added an offline `DeviceProfile` v1 schema, example and simpler browser draft on `/` (research v5 retained under `/lab/v5`). One canonical matrix: `docs/CONFIGURATION_FLOW.md`. No profile apply/NVS/live readback in firmware. At operator request **NO test suites, typechecks or heavy builds were executed for this sprint**; newly added source remains UNVERIFIED until a separately requested qualification.

## 2026-11-07 IPO Foundation v2 source-only

Added bounded DeviceProfile v2 schema/example, driver catalog, prioritized signals, typed process-to-output links, import/migration, new browser homepage and unlinked portable C++17 DTO/graph arbiter. Canonical `docs/CONFIGURATION_FLOW.md`. No production runtime/physical output changes. No tests/typecheck/builds run per operator direction; code not executable-qualified. Firmware v2 parser/NVS/apply/readback remains OPEN.

## 2026-11-07 IPO Runtime Bridge 1/2 — scoped evidence

Source changes introduce shared typed Mint-compatible runtime schedule owner, read-only console status, CrowPanel read-only graph admission/priority freshness selector and an **uninvoked** bounded full-profile wire codec + dual-slot NVS journal. This is not an applied v2 user profile. No new tests, typechecks, heavy builds or physical deployment per operator direction; all executable gates for this new source remain OPEN/UNVERIFIED. Do not confuse adding NVS code with durable profile activation or RF verification. Checkpoint: `docs/checkpoints/2026-11-07-bloomml-ipo-runtime-bridge.md`.

## 2026-11-07 IPO Runtime Bridge 3 — NOT QUALIFIED

Strict cJSON device-profile v2 decoder (schema/fields/ID and CrowPanel admission) and dormant stage-only coordinator added; current in-RAM Mint schedule fingerprint included in read-only console status. Code is not called for active configuration; no test, build or hardware acceptance of these additions performed by explicit user instruction. Full JSON labels are not preserved by binary runtime snapshot. Do not mark production v2 enabled.
