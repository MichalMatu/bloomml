# Growbox ML Controller

Native ESP-IDF controller for an ESP32-S3 growbox with deterministic climate control, real sensors, RF433 outputs, durable telemetry, TinyML shadow/research tooling, simulation and browser configuration.

## Current state

The CrowPanel 2.9-inch e-ink display is physically operational and refreshes from the real runtime. The current blocker is the SCD41 measurement path: the sensor is detected as available, but no successful sample is being recorded. Debug that regression before continuing the Clay/menu/button port or controller tuning.

Canonical source branch: `main`.

Read first:

- `docs/CURRENT_STATUS.md` — exact active blocker and next steps;
- `docs/ARCHITECTURE.md` — runtime ownership and safety boundaries;
- `docs/PROJECT_ROADMAP.md` — ordered product work;
- `docs/HISTORY.md` / `docs/CHANGELOG.md` — compact historical evidence.

## Core architecture

Production control is intentionally conservative:

- deterministic Rule control is authoritative;
- ML remains shadow/research-only;
- `OutputSupervisor` is the only normal owner of configured physical outputs;
- requested, resolved, transport-executed and physically observed output state are distinct;
- unavailable transport must never be presented as successful physical execution;
- the display is observer-only and cannot become another controller/output owner.

The portable controller lives under `lib/environment_control/`. Native ESP-IDF runtime/application code lives under `src/`. The browser configurator lives under `web/`, and simulation/ML/twin tooling under `tools/ml/`.

## Hardware

Current growbox target: ESP32-S3 CrowPanel 2.9-inch e-paper, N8R8 profile.

Authorized growbox serial device: `/dev/cu.usbserial-1130`.

Never use `/dev/cu.usbserial-10`. Do not use `/dev/cu.usbserial-1120` without explicit authorization.

Useful hardware references:

- `docs/IO_MAP.md`
- `docs/RF433_DEVICE_CODES.md`
- `docs/HARDWARE_BRINGUP_CHECKLIST.md`
- `docs/ESP32_S3_SERIAL_PORT_RESET.md`
- `docs/SHELLY_POWER_FEEDBACK.md`

## Firmware stack

- ESP-IDF 5.5.4
- ESP32-S3 / C++17
- CMake / CTest host tests
- emlearn-compatible generated C inference
- deterministic safety supervisor
- SCD41, BLE/Xiaomi and other native input adapters
- RF433 output transport
- e-ink display runtime
- clang-format / clang-tidy / pre-commit quality gates

## Browser and scientific tooling

The repository also contains:

- React + TypeScript + Vite browser configurator (`web/`);
- deterministic Python simulation/training tooling;
- PyVista scientific 3D twin;
- trace/replay and model-comparison tooling.

Live demos:

- Interactive chamber: https://michalmatu.github.io/growbox-ml-controller/chamber-3d
- Hardware/JSON configurator: https://michalmatu.github.io/growbox-ml-controller/

## Repository layout

```text
components/                 vendored/native components
config/                     board/runtime profiles
lib/environment_control/    portable climate controller
schemas/                    controller contracts
src/                        native ESP-IDF application/runtime
test/                       C++ host tests
tests/                      Python/tooling tests
tools/ml/                   simulation, training, replay and twin
tools/panel/                local diagnostics/configuration panel
tools/serial/               serial capture/replay helpers
web/                        browser configurator and chamber UI
docs/                       current technical documentation
```

See `docs/PROJECT_LAYOUT.md` for more detail.

## Verification

Typical host/firmware flow:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements-lock.txt

python -m tools.ml.pipeline --quick
cmake -S test/host -B build/host-tests
cmake --build build/host-tests --parallel
ctest --test-dir build/host-tests --output-on-failure

idf.py -B build/idf -D GROWBOX_BOARD_PROFILE=crowpanel-esp32s3-2_9-n8r8 build
```

Frontend gate:

```bash
corepack enable
corepack prepare pnpm@11.10.0 --activate
pnpm --dir web install --frozen-lockfile
pnpm --dir web typecheck
pnpm --dir web lint
pnpm --dir web test
pnpm --dir web build
```

Repository-only work should use the sandbox flow in `docs/SANDBOX_EXECUTION_FLOW.md`. Mac-local builds, USB/serial, flashing and physical-board evidence use Local Agent according to `AGENTS.md`.

## Documentation policy

Only current specifications, operational guides and compact history belong in `docs/`. Completed phase handoffs, temporary prompts, implementation plans and superseded audits are intentionally kept in Git history rather than as live documentation.

## License

MIT. The vendored emlearn runtime retains its upstream MIT notice.
