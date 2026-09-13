# Hardware values that differ from the shared Growbox defaults.
growbox_cache_default(GROWBOX_I2C_SDA_GPIO STRING "21" "Stage27 shared I2C SDA GPIO")
growbox_cache_default(GROWBOX_I2C_SCL_GPIO STRING "38" "Stage27 shared I2C SCL GPIO")
growbox_cache_default(GROWBOX_SD_POWER_GPIO STRING "42" "Stage27 SD power-enable GPIO")

# On-board 2.9-inch CrowPanel e-paper wiring (DIE01129S001).
growbox_cache_default(GROWBOX_EINK_SCLK_GPIO STRING "12" "CrowPanel e-ink SPI clock GPIO")
growbox_cache_default(GROWBOX_EINK_MOSI_GPIO STRING "11" "CrowPanel e-ink SPI MOSI GPIO")
growbox_cache_default(GROWBOX_EINK_CS_GPIO STRING "45" "CrowPanel e-ink chip-select GPIO")
growbox_cache_default(GROWBOX_EINK_DC_GPIO STRING "46" "CrowPanel e-ink data/command GPIO")
growbox_cache_default(GROWBOX_EINK_RST_GPIO STRING "47" "CrowPanel e-ink reset GPIO")
growbox_cache_default(GROWBOX_EINK_BUSY_GPIO STRING "48" "CrowPanel e-ink busy GPIO")
growbox_cache_default(GROWBOX_EINK_POWER_GPIO STRING "7" "CrowPanel e-ink power-enable GPIO")
