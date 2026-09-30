# Performance handoff

Status: 2026-09-30. The first local build/test acceleration pass is complete and merged. Start the next performance phase from fresh `main` on an otherwise idle host.

## Completed optimization pass

PR #16 (`Speed up local BloomML builds`) is merged. It changed only build/test tooling defaults:

- `IDF_CCACHE_ENABLE ?= 1` is exported by the Makefile, so ESP-IDF uses ccache by default while callers can still override it.
- `HOST_BUILD_JOBS` increased from 2 to 4.

Measured on MacBook Air M1 / 8 GB at source revision `b8a8295edc7ec78d1fa9965123da16009b8c67d9`:

### ESP-IDF clean rebuild

- uncached clean build: 89.270 s
- first ccache fill build: 150.204 s
- subsequent clean rebuild with a warm cache: 43.767 s
- steady-state clean rebuild improvement: about 51%
- second ccache run: 1018 direct hits out of 1020 cacheable compilations

The first cache-fill is slower than the uncached build. This is expected; judge ccache by repeated development rebuilds, not its first population run.

### Host C++ gate

- clean `make HOST_BUILD_JOBS=2 test-host`: 88.106 s
- clean `make HOST_BUILD_JOBS=4 test-host`: 72.494 s
- improvement: about 17.7%

The PR passed clean-room GitHub CI for host tests, normal ESP-IDF firmware, CrowPanel firmware and web tests, including clang-tidy.

## Next performance phase: CPU and GPU, separated by workload

### Measurement rules

1. Benchmark only when no other repository is compiling, testing, installing dependencies or running a heavy Local Agent/ML task on the M1.
2. Pin the exact source revision and fixed input data/seed where applicable.
3. Separate cold, warm and incremental build numbers.
4. Prefer three comparable runs and report the median.
5. Record wall time, CPU utilization, peak RSS and swap pressure; use `/usr/bin/time -lp` on macOS where practical.
6. For ML/inference comparisons also record model/input shape, batch size, numeric precision and result-equivalence tolerance.
7. Change one variable at a time.

### CPU track

Profile the real workloads:

- ESP-IDF build and representative one-source incremental rebuild;
- `make test-host` with the new 4-job default;
- Python ML pipeline/training/inference/probe workloads that are actually used during development.

Do not increase host jobs beyond 4 until measurements prove a benefit without creating swap pressure.

### GPU track

BloomML is the primary repository where Apple GPU/MPS may be useful, but eligibility must be proven from the current ML stack before changing code or dependencies.

First inspect the active ML backend and hot operations. If the existing framework supports Apple MPS for the relevant model, benchmark the same workload on CPU and MPS with identical input/seed/precision constraints. Measure both startup/transfer overhead and steady-state throughput; small workloads can be slower on GPU.

ESP-IDF/C++ compilation, ccache, clang-tidy and ordinary host tests remain CPU workloads. Do not try to route compilation through the GPU.

## Deferred work

- GitHub Actions cache optimization is not part of this local-M1 pass. Revisit it only when CI wall time becomes a development bottleneck.
- More aggressive C++ job counts are deferred until the current 4-job default is observed under normal development load.
- Do not redesign build directories or clean semantics solely for performance without a measured problem.

## New-chat bootstrap

Read:

1. `AGENTS.md`
2. `docs/CURRENT_STATUS.md`
3. this file
4. fresh `agent-control` daemon/binding state

Then verify the host is idle before any CPU/GPU A/B run. The figures above are reference measurements; take a fresh baseline from current `main` before making another optimization.