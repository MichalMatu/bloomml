# Project configuration

| Path | Purpose |
|------|---------|
| `idf/sdkconfig.defaults` | ESP-IDF base defaults (all boards) |
| `idf/sdkconfig.defaults.n16r8` | Default profile: 16 MB flash + 8 MB octal PSRAM; PSRAM may be absent |
| `idf/sdkconfig.defaults.n8r8` | Elecrow CrowPanel profile: 8 MB flash + 8 MB octal PSRAM |
| `idf/sdkconfig.defaults.n32r16v` | 32 MB flash + 16 MB octal PSRAM module |
| `../schemas/` | ML / wire data contract (not board Kconfig) |

PSRAM-enabled profiles use `CONFIG_SPIRAM_USE_CAPS_ALLOC`: ordinary `malloc()` stays internal by default and allocations that intentionally belong in PSRAM use the ESP-IDF capability allocator (`heap_caps_malloc(..., MALLOC_CAP_SPIRAM)`). Do not enable both `CONFIG_SPIRAM_USE_MALLOC` and `CONFIG_SPIRAM_USE_CAPS_ALLOC`; they are mutually exclusive `CONFIG_SPIRAM_USE` choices.

The N16R8 development profile keeps `CONFIG_SPIRAM_IGNORE_NOTFOUND=y`, so it intentionally does not request `CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP`; ESP-IDF disables that optimization when missing PSRAM is allowed. The CrowPanel N8R8 profile requires PSRAM and may request Wi-Fi/LwIP PSRAM allocation.

Local `sdkconfig` is generated at the **repo root** by `idf.py` / `make build` and is gitignored.

Override defaults:

```bash
make build
# or
idf.py -B build/idf \
  -D SDKCONFIG_DEFAULTS="config/idf/sdkconfig.defaults;config/idf/sdkconfig.defaults.n16r8" \
  build
```
