# Agent notes — Growbox ML

## Repository identity

- repository: `MichalMatu/growbox-ml-controller`
- repository id: `growbox-ml-controller`
- agent binding: `815cf40f-8d2a-4e1f-b7cc-c0f4e37b6cb5`
- source branch: `main`
- control branch: `agent-control`
- publishing branch: `gh-pages`

Do not infer or substitute another repository. A different repository requires an explicit Chat Bridge rebind.

## Bootstrap

For a fresh task read, in order:

1. `AGENTS.md`
2. `docs/README.md`
3. `docs/CURRENT_STATUS.md`
4. `docs/ARCHITECTURE.md`
5. `docs/PROJECT_ROADMAP.md`
6. the specific subsystem document for the change; current display work uses `docs/DISPLAY_UI_PORT.md`

Then fetch fresh `main` and `agent-control:.agent/status/daemon.json` before any write or Local Agent task.

Historical Stage27/Stage28 handoffs, temporary prompts and superseded audits are not live documentation. Use `docs/HISTORY.md`, `docs/CHANGELOG.md` and Git history when historical evidence is needed.

Research documents such as `DATA_CONTRACT.md`, `CONFIG_MATRIX.*` and `docs/simulator/*` may describe the older v4 128-feature/15-output toolchain. Do not treat them as the production climate-v6 runtime contract unless the task explicitly targets that research toolchain.

## Work mode

Use direct GitHub for bounded source/config/documentation changes when the exact diff and CI can verify the result.

Use ChatGPT Sandbox for repository-only execution when matching source/dependency packs exist. Canonical details are in `docs/SANDBOX_EXECUTION_FLOW.md`.

Use Local Agent for Mac-local toolchains, local simulator assets not represented in the sandbox, USB/serial, flashing, physical-board observation and other machine-specific evidence.

Every executable Local Agent task for this repository must contain exactly:

```json
{
  "agent_binding": "815cf40f-8d2a-4e1f-b7cc-c0f4e37b6cb5",
  "work_branch": "main",
  "resources": []
}
```

Task ids/payloads are immutable. Read terminal `.agent/results/<task-id>.json` before claiming success. Never launch or delegate to local Codex from a Local Agent task.

`agent-control` is control-plane state only; never use it for product code. Do not race direct writes against an active Local Agent task on the same source branch.

## Hardware boundaries

Authorized growbox serial device: `/dev/cu.usbserial-1130`.

Never open, probe, monitor, reset or flash `/dev/cu.usbserial-10`.
Do not use `/dev/cu.usbserial-1120` without explicit authorization.

For unattended qualification keep physical outputs disabled unless the operator explicitly changes that instruction.

## Frozen ownership/safety rules

- production Rule control remains authoritative;
- ML remains shadow/research-only;
- `OutputSupervisor` is the only normal owner of configured physical outputs;
- unavailable transport must not fabricate successful execution;
- one-way RF completion is transport evidence, not physical acknowledgement;
- raw RF remains restricted to explicit maintenance handling;
- lamp thermal trip remains `>= 28 C`;
- lamp recovery remains `<= 26 C` continuously for 10 minutes;
- e-ink/UI code is observer-side and must not mutate control or safety state;
- slow display work must stay outside the control hot path.

## Current display-port boundary

`vendor/litegraph_epd_port/` is read-only donor/reference material and must stay excluded from builds.

The current production application is C++17; pinned Clay 0.14 requires C++20. Follow `docs/DISPLAY_UI_PORT.md`: prefer an isolated C++20 component with Clay-free public structs. Do not replace the native SSD1680 backend or copy Arduino hardware ownership into production.

## Panel UI layout rule

Inside pot/actuator/target cards, related fields stay horizontal. Do not turn compact multi-field cards into vertical stacks or full-width mini-cells.

Canonical patterns:

- sensors: `.pot-card-sensors`;
- targets/cultivation: `.compact-row` + `.mini-cell`;
- actuators: horizontal `.field-stack`;
- pot cards: common `--pot-card-w` sizing.

Page layout:

- left column: `card-stack` with Control, Sensors, Targets and Actuators;
- right column: live sensor/output state and `panel-actions`;
- previous state and other panel-action views open in one movable panel modal: `#modal-backdrop` -> `.panel-modal.modal--wide`.

After changing `tools/panel/static/js/form.js` or `tools/panel/static/panel.css` run:

```bash
.venv/bin/python -m pytest tests/test_panel_layout.py -q
```

## Completion rule

A commit proves publication, not execution. Report the exact commit plus relevant completed checks. Hardware claims require exact-SHA physical evidence.
