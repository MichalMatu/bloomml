#!/usr/bin/env bash
set -euo pipefail

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

PY="${ROOT}/.venv/bin/python"
if [[ ! -x "$PY" ]]; then
  PY="$(command -v python3)"
fi

echo "==> output execution ownership"
"$PY" "${ROOT}/scripts/check_output_rf_ownership.py"

echo "==> runtime configuration SSOT"
"$PY" "${ROOT}/scripts/check_runtime_config_ssot.py"

echo "==> runtime composition boundaries"
"$PY" "${ROOT}/scripts/check_runtime_boundaries.py"

echo "==> service console boundaries"
"$PY" "${ROOT}/scripts/check_service_console_boundaries.py"

echo "==> app-mode build boundaries"
"$PY" "${ROOT}/scripts/check_app_mode_boundaries.py"

echo "architecture gate: OK"
