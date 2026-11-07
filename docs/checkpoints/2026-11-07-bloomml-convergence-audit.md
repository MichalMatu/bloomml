# Checkpoint — product direction and bounded repair (2026-11-07)

Main source before changes: `d5044b712b9f7b56278c31f5770d853836089071`.
Research branch observed: `73de11be282d40d03767275cc6a0891e58ed23f8`; kept untouched.
Local Agent: daemon 4.20.16, idle, no active task at observation time.

Decision: BloomML is ONE future operator product, with a versioned production hardware/targets profile, offline safe controller and monitoring. The serial v4 demo, browser v5/3D and research physics/ML/MPC are supporting developer tools. Do not confuse their contracts with production climate-v6, and do not activate unqualified ML.

Repairs: serial panel same-origin/JSON/body-limit checks with 8 new test cases (one parameterized) plus browser v5 label/filename corrections; README/status/architecture direction aligned. No physical control, Rule, OutputSupervisor or model execution path changed.

Checks: panel-focused 62 pytest PASS in sandbox with import-only pyserial stub and archived source; web offline `pnpm gate`: typecheck PASS, lint PASS, 104 Vitest PASS, Vite build PASS (large 3D bundle warning). This archive is older than current main; these are narrow tests, not a complete current-HEAD qualification. Hardware/IDF/real Kaggle MPC: NOT RUN.

Open: exact-main clean rebuild/gates, full repository audit, physical SCD41/buttons, one production DeviceProfile contract, research MPC 19/22 hard scenario failures and latest-patch retest, real-data calibration. See `../AUDIT_STATUS.md` and `../NEXT_CHAT.md`.
