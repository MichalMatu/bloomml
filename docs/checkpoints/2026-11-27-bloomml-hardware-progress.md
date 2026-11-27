# 2026-11-27 — CrowPanel hardware progress

H0 PASS; H1 PARTIAL; H2/H3 NOT ADMITTED; IPO ACTIVATION CLOSED.

| Evidence | Source revision | Result |
| --- | --- | --- |
| Normal sensor/display boot | 0015a77 | PASS, isolated fake-locked |
| Initial RF trial | 0015a77 | FAIL, main stack overflow; safe firmware restored |
| RF decode workspace repair | d68f059 | 2/2 host tests and build PASS; isolated 165 s capture, one boot, main margin 5472 B, four TX, no TX errors; safe restore PASS |
| Telemetry scratch | a506cff | Host and build PASS, hardware margin 2076 B (previous 1052 B), no storage write/queue errors |
| Main budget unified | 2199a75 | Both build variants PASS; fake-locked hardware PASS, main 5472 B, telemetry 2076 B, no observed restart/write/queue error |

All hardware captures are bounded; RF TX is not receiver acknowledgement. SD mount timeout was observed; operator subsequently confirmed no SD card was inserted. SD qualification is DEFERRED until a card is available. Internal flash fallback works. No energy measurement performed. Last observed exact firmware 2199a75, fake-locked.

Native NVS fault/recovery: **SKIPPED / NOT EXECUTED — TOOL BLOCK**. Operator requested separate local follow-up. Existing mock NVS tests remain software evidence only. Do not treat this as hardware PASS or repeat the blocked task in this workflow. Physical power interruption, stale/invalid inputs, thermal trip/recovery and receiver acknowledgement are still unqualified. Production closed gate unchanged. Remaining hardware qualification is deferred, not passed. Independent telemetry scratch regression `a813a25` passed 3/3 host suites; software iteration closed within tested scope.

Evidence: agent-control results for la-bloomml-crowpanel-130-boot-20261127-v1, la-bloomml-h1-rf-isolated-20261127-v1, la-bloomml-rf-stack-hardware-retest-20261127-v1, la-bloomml-storage-stack-hardware-retest-20261127-v1 and la-bloomml-main-stack-hardware-20261127-v1.
