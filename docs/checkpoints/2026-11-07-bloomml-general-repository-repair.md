# BloomML — General repository audit and repair checkpoint (2026-11-07)

## Source identity and preflight

- Repo: `MichalMatu/bloomml`; source `main`, Local Agent transport `agent-control`, publication `gh-pages`. Entered at clean `54c486dae121517161efd6d3d3498e5b4623e1fc`. Three repairs on main end at code SHA `4aadf5fa9c0aa26a221d7a1b8d12ad4082ce4793`; fetch the current HEAD because this docs commit is later.
- Inventory: 911 files, 148 commits over last 7 days, 148/30 days, 968/90 days from Mac Local Agent. Recent product work emphasizes IPO v2 source/diagnostics and documentation; research MPC stays shadow-only.
- Authorized Local Agent binding `815cf40f-8d2a-4e1f-b7cc-c0f4e37b6cb5`, `work_branch=main`, `resources=[]`; tests used clean temporary clones and did not overwrite local uncommitted work.

## Reproduced defects, fixes and tests

| Reproduced problem | Fix | Regression evidence |
| --- | --- | --- |
| Four IPO buttons used unsupported `size="sm"` | Remove four overrides, `cf1c434` | Typecheck passes in final web gate |
| 96 strict ESLint violations from feature-local `className` styling | Centralize in 40 typed `app-chrome` presets, preserve DOM and CSS, `196c305` | ESLint and UI consistency Vitest pass |
| Side-effect `GET /api/diagnostics?refresh=1` bypassed protected POST-origin check | Side-effect GET now 405; same-origin POST refresh, `cf1c434` | HTTP cross-site and same-site tests |
| Cached diagnostics could satisfy a new serial refresh despite no new UART frame | Sequence-based freshness, serialized refresh and HTTP 504 on timeout, `4aadf5f` | Stale/fresh/timeout regression tests |
| Browser text incorrectly claimed no IPO NVS/readback | Distinguish diagnostic NVS/stored/active readback from blocked apply | IPO frontend tests pass |

## Exact-source qualification

- Preflight `54c486d`: `make test-ipo` **3/3 PASS**, Python `compileall` PASS. Web typecheck **FAIL** with four TS2322 errors; reproduced before repair. Task result `agent-control:.agent/results/la-bloomml-general-audit-preflight-20261107-v1.json`.
- Repair `cf1c434`: `make test-host HOST_BUILD_JOBS=2` including display/portable C++ **PASS**, Python compileall and diff-check PASS, clean source. Web typecheck recovered but 96 ESLint violations surfaced. Task result `la-bloomml-general-audit-repair-gates-20261107-v1`.
- Web repair `196c305`: frozen offline pnpm install then `pnpm gate` **PASS**: TypeScript, lint, **114/114 Vitest** across 13 files, Vite production build. Nonfatal oversized 3D chunk remains. Clean pushed temp worktree. Task result `la-bloomml-general-audit-web-style-owner-repair-20261107-v1`.
- Panel repair `4aadf5f`: isolated Mac venv with pytest, pyserial and numpy, **135/135 pytest PASS** (7 panel modules), `git diff --check` PASS, clean worktree. First minimal environment lacked numpy via shared `tests/conftest.py`; second venv installed it, without weakening tests. Task result `la-bloomml-general-audit-panel-qa-20261107-v2`.
- Compare starting SHA to `4aadf5f`: 3 code commits affecting 15 files. No production `src/climate`/Rule/OutputSupervisor or NVS code changed; IPO C++ change was only a comment. No hardware commands executed.

## Evidence gaps / blocked gates

- Authorized `/dev/cu.usbserial-120` **ABSENT** in Mac preflight. No flash, actual UART, RF433, button, sensor or physical isolation test. `/dev/cu.usbserial-10` prohibited; `/dev/cu.usbserial-1120` requires separate approval.
- Prior CrowPanel ESP-IDF build PASS is tied to old SHA `1a1b3de8`; no exact-HEAD firmware build here (`idf.py` absent on Mac PATH). `clang-format` and `clang-tidy` missing on PATH.
- ChatGPT Linux Sandbox source archive in Library `growbox-source-deb687...` predates current main; exact-main software QA was run on Mac instead, not falsely labeled Linux. No full 911-file correctness proof, security advisory/dependency audit, dedicated formatter gate, manual 3D visual sign-off or complete browser-to-real-device E2E.
- Research MPC historical hard scenario 19/22 FAIL remains isolated. Active Mint/Rule/OutputSupervisor unchanged. Production `ClosedIpoActivationGate` remains CLOSED; stored NVS is not active output authority.

## Closure / NEXT_CHAT

- Closed: all confirmed safe software repairs listed above; no new product features, no dropped tests, no branch recreation, no GitHub Actions or force push. Maintain historical test SHA evidence; rerun only tests affected by new changes.
- Software follow-ups if warranted: exact-main Linux pack and reproducible clean-room runs, dependency audit, formatter/IDF toolchain visibility, optional real-browser visual QA. Do not confuse these with physical H0.
- **Single next physical step H0:** connect explicitly authorized CrowPanel and independently establish physical isolation of every output. If still absent, `BLOCKED / HARDWARE_ABSENT`; do not flash, issue RF, or enable IPO apply. H1/H2/H3 only after approved, fake-locked physical acceptance.
- Resume by fetching fresh `main` HEAD and agent daemon state, then read `AGENTS.md`, `docs/CURRENT_STATUS.md`, `docs/CONFIGURATION_FLOW.md` and `docs/NEXT_CHAT.md`. No reauditing of closed fixes absent regression evidence.
