#!/usr/bin/env bash
set -euo pipefail

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

HOST_BUILD_JOBS="${HOST_BUILD_JOBS:-2}"
if [[ ! "${HOST_BUILD_JOBS}" =~ ^[1-9][0-9]*$ ]]; then
  echo "HOST_BUILD_JOBS must be a positive integer, got: ${HOST_BUILD_JOBS}" >&2
  exit 2
fi

run_suite() {
  local name="$1"
  local source_dir="$2"
  local build_dir="build/host-${name}"

  echo "==> ${name}"
  cmake -S "$source_dir" -B "$build_dir"
  cmake --build "$build_dir" --parallel "$HOST_BUILD_JOBS"
  ctest --test-dir "$build_dir" --output-on-failure
}

run_suite "display-presenter" "test/test_display_presenter"
run_suite "display-runtime" "test/test_display_runtime"
run_suite "display-observer" "test/test_display_observer"
run_suite "display-raster" "test/test_display_raster"
run_suite "display-async" "test/test_display_async"
run_suite "clay-ui" "lib/growbox_clay_ui"
