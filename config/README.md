# Project configuration

| Path | Purpose |
|------|---------|
| `idf/sdkconfig.defaults` | ESP-IDF base defaults (all boards) |
| `idf/sdkconfig.defaults.n16r8` | Default profile: 16 MB flash + 8 MB octal PSRAM; PSRAM may be absent |
| `idf/sdkconfig.defaults.n8r8` | Elecrow CrowPanel profile: 8 MB flash + 8 MB octal PSRAM |
| `idf/sdkconfig.defaults.n32r16v` | 32 MB flash + 16 MB octal PSRAM module |
| `idf/sdkconfig.defaults.stage27` | Real-input Stage27 overlay: NimBLE observer/central host + logging/FAT settings |
| `idf/sdkconfig.defaults.stage27c` | CrowPanel Stage27C storage/partition/diagnostic overlay |
| `idf/sdkconfig.defaults.stage28rf` | Optional RF-enabled main-stack overlay |
| `../schemas/` | ML / wire data contract (not board Kconfig) |

PSRAM-enabled profiles use `CONFIG_SPIRAM_USE_CAPS_ALLOC`: ordinary `malloc()` stays internal by default and allocations that intentionally belong in PSRAM use the ESP-IDF capability allocator (`heap_caps_malloc(..., MALLOC_CAP_SPIRAM)`). Do not enable both `CONFIG_SPIRAM_USE_MALLOC` and `CONFIG_SPIRAM_USE_CAPS_ALLOC`; they are mutually exclusive `CONFIG_SPIRAM_USE` choices.

The N16R8 development profile keeps `CONFIG_SPIRAM_IGNORE_NOTFOUND=y`, so it intentionally does not request `CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP`; ESP-IDF disables that optimization when missing PSRAM is allowed. The CrowPanel N8R8 profile requires PSRAM and may request Wi-Fi/LwIP PSRAM allocation.

## Canonical CrowPanel production build

Do not reconstruct the production real-input CrowPanel configuration from only the base + N8R8 defaults. That omits the Stage27 NimBLE configuration and Stage27C storage/partition settings.

Use:

```bash
make build-crowpanel
```

This delegates to `scripts/stage27c_crowpanel.sh` and composes:

```text
config/idf/sdkconfig.defaults
config/idf/sdkconfig.defaults.n8r8
config/idf/sdkconfig.defaults.stage27
config/idf/sdkconfig.defaults.stage27c
```

with board `crowpanel-esp32s3-2_9-n8r8`, runtime profile `stage27c-crowpanel`, app mode `climate-v6-real-inputs`, and e-ink enabled by the Make target. If RF433 loopback is explicitly enabled, the script also adds `sdkconfig.defaults.stage28rf`.

`make build-crowpanel` is the required compile gate for production display/Clay changes. Generic `make build` defaults to the normal N16R8 development path and does not prove that the CrowPanel real-input display path compiles.

Local `sdkconfig` for the generic build is generated at the repo root by `idf.py` / `make build` and is gitignored. The Stage27C script uses a build-directory-specific sdkconfig by default.

Generic override example:

```bash
make build
# or
idf.py -B build/idf \
  -D SDKCONFIG_DEFAULTS="config/idf/sdkconfig.defaults;config/idf/sdkconfig.defaults.n16r8" \
  build
```
