# Performance handoff

Status: **2026-09-30 — performance work is paused until the MacBook M1 Pro / 32 GB host is available.** Resume from fresh `main`; do not change ML/build dependencies or concurrency before a fixed baseline.

## Stable optimizations already merged

PR #16 (`Speed up local BloomML builds`) changed tooling only:

- ESP-IDF ccache is enabled by default through `IDF_CCACHE_ENABLE ?= 1`;
- `HOST_BUILD_JOBS` default is 4.

Historical MacBook Air M1 / 8 GB measurements at source `b8a8295edc7ec78d1fa9965123da16009b8c67d9`:

- ESP-IDF uncached clean build: **89.270 s**;
- first ccache-fill build: **150.204 s**;
- warm-cache clean rebuild: **43.767 s**;
- clean `make HOST_BUILD_JOBS=2 test-host`: **88.106 s**;
- clean `make HOST_BUILD_JOBS=4 test-host`: **72.494 s**.

The first ccache population is slower by design; judge it on repeated development rebuilds. Keep 4 host jobs until a fresh baseline on the 32 GB machine proves that more parallelism is useful.

## ML/GPU eligibility audit completed before the pause

The active research trainer uses TensorFlow/Keras with a small dense MLP. The current quick/full dataset presets are small by desktop-GPU standards, training is deterministic, and the training path explicitly limits TensorFlow CPU threading to one inter-op and one intra-op thread.

Current dependency state does **not** pin `tensorflow-metal`. Apple GPU availability must therefore be probed on the new host rather than assumed.

No CPU-vs-GPU A/B benchmark was run before the pause. ML remains shadow/research-only and is not production control authority.

## Resume on the M1 Pro / 32 GB host

For an apples-to-apples build comparison, first rerun the historical build/test commands at exact source `b8a8295edc7ec78d1fa9965123da16009b8c67d9` on the new machine. Then establish a fresh current-`main` baseline.

For build/test measurements:

1. run one heavy workload at a time on an otherwise idle host;
2. separate cold, warm-cache and representative incremental paths;
3. use at least three comparable runs where practical and report medians;
4. record wall time, CPU, process-tree peak RSS and swap/memory pressure;
5. keep the first cross-host run at the current 4-job default;
6. only after that test higher job counts one variable at a time.

For ML CPU/GPU A/B:

1. first record TensorFlow version, installed Metal plugin state and `tf.config.list_physical_devices()`;
2. do not install or change a GPU dependency before recording the existing-environment CPU baseline;
3. compare the same dataset, model, seed, batch size, precision and training configuration;
4. separate startup/device-transfer overhead from steady-state training/inference time;
5. verify result equivalence within an explicit tolerance;
6. keep GPU support only if the real workload is faster enough to justify the extra dependency/tooling surface.

ESP-IDF/C++ compilation, ccache, clang-tidy and ordinary host tests remain CPU/RAM/cache/I/O workloads and must not be routed through GPU-specific tooling.

## Bootstrap after the pause

Read `AGENTS.md`, `docs/CURRENT_STATUS.md`, this file, then fetch fresh `main` and the Local Agent daemon state. Historical timings are reference values only; future development starts from fresh `main`.
