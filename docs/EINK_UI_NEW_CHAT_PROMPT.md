# Fresh-chat prompt — autonomous e-ink UI implementation

Copy the block below into a new ChatGPT window to start the implementation.

```text
[LA_AGENT=815cf40f-8d2a-4e1f-b7cc-c0f4e37b6cb5] [LA_REPO=growbox-ml-controller] [LA_REPOSITORY=MichalMatu/growbox-ml-controller] [LA_CHAT=chat-e23bf59b]

Work only on MichalMatu/growbox-ml-controller. Keep this repository/binding immutable for the whole task. Never infer, substitute, queue, cancel or execute another repository through Local Agent. Every Local Agent task JSON you create MUST contain exactly:
"agent_binding": "815cf40f-8d2a-4e1f-b7cc-c0f4e37b6cb5"
and use "work_branch": "main" plus "resources": [].

Continue autonomously from repository evidence. First read, in order:
1. AGENTS.md
2. docs/FRESH_CHAT_BOOTSTRAP.md
3. docs/CURRENT_STATUS.md
4. docs/ARCHITECTURE.md
5. docs/PROJECT_ROADMAP.md
6. docs/CONTINUATION_PLAN.md
7. docs/EINK_UI_HANDOFF.md

Then fetch fresh main HEAD and agent-control:.agent/status/daemon.json before any write. Check for an active Local Agent task before editing main.

TASK: implement the e-ink operator UI described in docs/EINK_UI_HANDOFF.md before starting Controller behavior quality.

Reference implementation is MichalMatu/esp32s3_LiteGraph, pinned reference observed during handoff preparation at 50716ee5cc4f4c426ba19ea5f137d944db2970c3. The user explicitly confirms that the e-ink/button module and pin map are 100% the same as the growbox board. Reuse the proven Clay menu, button navigation, simulator, refresh policy and generic display infrastructure rather than redesigning them. Adapt only the growbox-specific data/presenters.

Target UI: useful home/glance view plus button menu pages for Environment, Outputs, System and Diagnostics. Show temperature, RH, CO2, RTC/time, automation/controller state, lamp/fan/humidifier state with correct intent/executed/transport semantics, safety state/reason, storage/BLE/input health, firmware identity and bounded memory diagnostics. The display is read-only/observer-side and must not become an output owner or safety/configuration source of truth.

Work autonomously while the user sleeps. ChatGPT plans; Local Agent executes deterministic Mac-local commands, builds, simulator, serial/USB and device operations. NEVER launch/delegate to local Codex from Local Agent. Use direct GitHub only for bounded edits that exact diff + CI can verify. Be creative in finding bugs, but do not broaden into an architecture rewrite.

For every problem found: diagnose it, make the smallest justified fix, rerun the focused test, then continue. Do not repeatedly rerun the same test without new information. Keep main clean and preserve all current architecture/safety invariants.

UNATTENDED HARDWARE SAFETY:
- only /dev/cu.usbserial-1130 is authorized;
- never touch /dev/cu.usbserial-10;
- do not use /dev/cu.usbserial-1120 without explicit authorization;
- keep GROWBOX_RF433_LOOPBACK_ENABLED=0;
- keep GROWBOX_STAGE28_REAL_OUTPUTS_ENABLED=0;
- keep GROWBOX_STAGE28_THERMAL_TEST_SEQUENCE_ENABLED=0;
- real inputs/RTC/SD/BLE may remain enabled.

Verification is mandatory, not just compilation. Port/add host tests for Clay/menu/button navigation/presenter state and use the simulator. Run applicable pre-commit/guards, full host C++ tests, relevant Python tests, clang-tidy and the real ESP-IDF growbox build. Compare firmware size and memory with a pre-change baseline. Verify canonical GitHub CI/Sandbox on the exact final SHA.

After software is green, flash that exact SHA to /dev/cu.usbserial-1130 and observe a meaningful boot + soak. Inspect logs continuously for Guru Meditation, panic, abort, watchdog, stack overflow, brownout, reboot loops, unexpected reset reasons, display/SPI errors, sensor/SD/BLE regressions and control-loop starvation. Capture internal free heap, minimum-ever free heap, largest free block and PSRAM usage when available; inspect relevant stack high-water marks when accessible. There must be stable headroom, no monotonic heap leak and no suspicious fragmentation/stack regression.

Do not claim physical button UX was human-verified if nobody pressed the buttons while unattended; simulator/host input tests can qualify navigation, and leave only a tiny morning tactile/visual sanity check if physical pressing cannot be automated. The implementation itself should still be complete and flashed.

Update docs/CURRENT_STATUS.md, docs/CHANGELOG.md, docs/PROJECT_ROADMAP.md, docs/CONTINUATION_PLAN.md and docs/EINK_UI_HANDOFF.md with exact final SHA/test/CI/hardware evidence. End with clean main and no junk branch. Do NOT start Controller behavior quality afterward.

Do not stop for ordinary engineering choices. Stop only for a real safety blocker, repository identity conflict, unavailable required hardware, or an ambiguity that cannot be resolved from repository/reference evidence. If blocked, report exact evidence rather than claiming success.
```
