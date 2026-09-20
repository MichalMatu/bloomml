include_guard(GLOBAL)

function(growbox_cache_default name type value description)
  if(NOT DEFINED ${name})
    set(${name} "${value}" CACHE ${type} "${description}")
  else()
    set(${name} "${${name}}" CACHE ${type} "${description}")
  endif()
endfunction()

function(growbox_require_bool name)
  if(NOT "${${name}}" MATCHES "^[01]$")
    message(FATAL_ERROR "${name} must be 0 or 1, got '${${name}}'")
  endif()
endfunction()

function(growbox_require_assigned_gpio name reason)
  if(NOT "${${name}}" MATCHES "^[0-9]+$")
    message(FATAL_ERROR "${name} must be an assigned non-negative GPIO (${reason}), got '${${name}}'")
  endif()
endfunction()

function(growbox_require_distinct_gpio left_name right_name reason)
  if("${${left_name}}" STREQUAL "${${right_name}}")
    message(FATAL_ERROR
      "GPIO conflict (${reason}): ${left_name}=${${left_name}} and ${right_name}=${${right_name}}")
  endif()
endfunction()

get_filename_component(GROWBOX_RUNTIME_CONFIG_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

growbox_cache_default(GROWBOX_BOARD_PROFILE STRING "esp32s3-devkitc1-n16r8"
                      "Growbox hardware board profile")
growbox_cache_default(GROWBOX_RUNTIME_PROFILE STRING "generic"
                      "Growbox runtime/deployment profile")

set(_growbox_board_profile_file
    "${GROWBOX_RUNTIME_CONFIG_ROOT}/config/boards/${GROWBOX_BOARD_PROFILE}.cmake")
if(NOT EXISTS "${_growbox_board_profile_file}")
  message(FATAL_ERROR "Unknown GROWBOX_BOARD_PROFILE '${GROWBOX_BOARD_PROFILE}'")
endif()
include("${_growbox_board_profile_file}")

# Shared hardware defaults. Board profiles set only values that genuinely differ.
growbox_cache_default(GROWBOX_SD_MOSI_GPIO STRING "40" "Stage27 SD SPI MOSI GPIO")
growbox_cache_default(GROWBOX_SD_MISO_GPIO STRING "13" "Stage27 SD SPI MISO GPIO")
growbox_cache_default(GROWBOX_SD_SCLK_GPIO STRING "39" "Stage27 SD SPI clock GPIO")
growbox_cache_default(GROWBOX_SD_CS_GPIO STRING "10" "Stage27 SD SPI chip-select GPIO")
growbox_cache_default(GROWBOX_RF433_TX_GPIO STRING "8" "Stage28 RF433 TX GPIO")
growbox_cache_default(GROWBOX_RF433_RX_GPIO STRING "14" "Stage28 RF433 RX GPIO")

set(_growbox_runtime_profile_file
    "${GROWBOX_RUNTIME_CONFIG_ROOT}/config/runtime/profiles/${GROWBOX_RUNTIME_PROFILE}.cmake")
if(NOT EXISTS "${_growbox_runtime_profile_file}")
  message(FATAL_ERROR "Unknown GROWBOX_RUNTIME_PROFILE '${GROWBOX_RUNTIME_PROFILE}'")
endif()
include("${_growbox_runtime_profile_file}")

# Generic fallbacks apply only when neither CLI nor selected profile supplied a value.
growbox_cache_default(GROWBOX_BLE_TP357_MAC STRING "" "Stage27 TP357 inside sensor MAC")
growbox_cache_default(GROWBOX_BLE_XIAOMI_MAC STRING "" "Stage27 Xiaomi nearby BTHome sensor MAC")
growbox_cache_default(GROWBOX_STAGE27_SD_ENABLED STRING "0" "Enable Stage27 SD telemetry backend")
growbox_cache_default(GROWBOX_STAGE27_FLASH_FALLBACK_ENABLED STRING "0"
                      "Enable Stage27 internal-flash telemetry fallback")
growbox_cache_default(GROWBOX_SD_CMD0_PRECONDITION STRING "0"
                      "Enable CrowPanel SD CMD0 compatibility precondition")
growbox_cache_default(GROWBOX_RF433_LOOPBACK_ENABLED STRING "0"
                      "Enable Stage28 RF433 loopback transport")
growbox_cache_default(GROWBOX_RF433_REMOTE_CAPTURE_ENABLED STRING "0"
                      "Enable Stage28C passive remote capture diagnostics")
growbox_cache_default(GROWBOX_EINK_DISPLAY_ENABLED STRING "0"
                      "Enable read-only e-ink status display integration")
growbox_cache_default(GROWBOX_EINK_SCLK_GPIO STRING "-1" "E-ink display SPI clock GPIO")
growbox_cache_default(GROWBOX_EINK_MOSI_GPIO STRING "-1" "E-ink display SPI MOSI GPIO")
growbox_cache_default(GROWBOX_EINK_CS_GPIO STRING "-1" "E-ink display chip-select GPIO")
growbox_cache_default(GROWBOX_EINK_DC_GPIO STRING "-1" "E-ink display data/command GPIO")
growbox_cache_default(GROWBOX_EINK_RST_GPIO STRING "-1" "E-ink display reset GPIO")
growbox_cache_default(GROWBOX_EINK_BUSY_GPIO STRING "-1" "E-ink display busy GPIO")
growbox_cache_default(GROWBOX_EINK_POWER_GPIO STRING "-1" "E-ink display power-enable GPIO")
growbox_cache_default(GROWBOX_EINK_KEY_HOME_GPIO STRING "-1" "E-ink Home key GPIO")
growbox_cache_default(GROWBOX_EINK_KEY_BACK_GPIO STRING "-1" "E-ink Back key GPIO")
growbox_cache_default(GROWBOX_EINK_KEY_PREVIOUS_GPIO STRING "-1" "E-ink Previous key GPIO")
growbox_cache_default(GROWBOX_EINK_KEY_NEXT_GPIO STRING "-1" "E-ink Next key GPIO")
growbox_cache_default(GROWBOX_EINK_KEY_OK_GPIO STRING "-1" "E-ink OK key GPIO")
growbox_cache_default(GROWBOX_STAGE28_SERVICE_CONSOLE_ENABLED STRING "1"
                      "Enable Stage28 primary-serial service console")
growbox_cache_default(GROWBOX_STAGE28_REAL_OUTPUTS_ENABLED STRING "0"
                      "Enable bounded Stage28 real RF outputs")
growbox_cache_default(GROWBOX_STAGE28_THERMAL_TEST_SEQUENCE_ENABLED STRING "0"
                      "Enable retired Gate6 thermal test compatibility switch")
growbox_cache_default(GROWBOX_STAGE28E_LOG_COMPILE_LEVEL STRING "2"
                      "Stage28E diagnostic compile-time log level 0=ERROR..4=TRACE")
growbox_cache_default(GROWBOX_STAGE28E_BREADCRUMB_RESTART_SELFTEST STRING "0"
                      "Run one bounded Stage28E breadcrumb software-restart self-test")

if(NOT DEFINED GROWBOX_FIRMWARE_GIT_SHA)
  execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY "${GROWBOX_RUNTIME_CONFIG_ROOT}"
    OUTPUT_VARIABLE _growbox_detected_git_sha
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE _growbox_git_sha_status
  )
  if(NOT _growbox_git_sha_status EQUAL 0 OR _growbox_detected_git_sha STREQUAL "")
    set(_growbox_detected_git_sha "unknown")
  endif()
  set(GROWBOX_FIRMWARE_GIT_SHA "${_growbox_detected_git_sha}" CACHE STRING
      "Exact Git SHA embedded in Stage27 firmware")
else()
  set(GROWBOX_FIRMWARE_GIT_SHA "${GROWBOX_FIRMWARE_GIT_SHA}" CACHE STRING
      "Exact Git SHA embedded in Stage27 firmware")
endif()

foreach(_growbox_bool_var IN ITEMS
    GROWBOX_STAGE27_SD_ENABLED
    GROWBOX_STAGE27_FLASH_FALLBACK_ENABLED
    GROWBOX_SD_CMD0_PRECONDITION
    GROWBOX_RF433_LOOPBACK_ENABLED
    GROWBOX_RF433_REMOTE_CAPTURE_ENABLED
    GROWBOX_EINK_DISPLAY_ENABLED
    GROWBOX_STAGE28_SERVICE_CONSOLE_ENABLED
    GROWBOX_STAGE28_REAL_OUTPUTS_ENABLED
    GROWBOX_STAGE28_THERMAL_TEST_SEQUENCE_ENABLED
    GROWBOX_STAGE28E_BREADCRUMB_RESTART_SELFTEST)
  growbox_require_bool(${_growbox_bool_var})
endforeach()

if(GROWBOX_STAGE28_THERMAL_TEST_SEQUENCE_ENABLED AND NOT GROWBOX_STAGE28_REAL_OUTPUTS_ENABLED)
  message(FATAL_ERROR
    "GROWBOX_STAGE28_THERMAL_TEST_SEQUENCE_ENABLED requires GROWBOX_STAGE28_REAL_OUTPUTS_ENABLED=1")
endif()
if(GROWBOX_STAGE28_REAL_OUTPUTS_ENABLED AND NOT GROWBOX_RF433_LOOPBACK_ENABLED)
  message(FATAL_ERROR
    "GROWBOX_STAGE28_REAL_OUTPUTS_ENABLED requires GROWBOX_RF433_LOOPBACK_ENABLED=1")
endif()
if(GROWBOX_EINK_DISPLAY_ENABLED AND NOT GROWBOX_APP_CLIMATE_V6_REAL_INPUTS)
  message(FATAL_ERROR
    "GROWBOX_EINK_DISPLAY_ENABLED requires GROWBOX_APP_MODE=climate-v6-real-inputs")
endif()

# Pins that are simultaneously active in the real-input runtime must never alias.
if(GROWBOX_APP_CLIMATE_V6_REAL_INPUTS)
  growbox_require_distinct_gpio(GROWBOX_I2C_SDA_GPIO GROWBOX_I2C_SCL_GPIO "I2C bus")

  if(GROWBOX_RF433_LOOPBACK_ENABLED)
    growbox_require_distinct_gpio(GROWBOX_RF433_TX_GPIO GROWBOX_RF433_RX_GPIO "RF433 transport")
    foreach(_growbox_i2c_pin IN ITEMS GROWBOX_I2C_SDA_GPIO GROWBOX_I2C_SCL_GPIO)
      foreach(_growbox_rf_pin IN ITEMS GROWBOX_RF433_TX_GPIO GROWBOX_RF433_RX_GPIO)
        growbox_require_distinct_gpio(${_growbox_i2c_pin} ${_growbox_rf_pin}
                                      "I2C and RF433 enabled together")
      endforeach()
    endforeach()
  endif()

  if(GROWBOX_STAGE27_SD_ENABLED)
    set(_growbox_sd_pins
        GROWBOX_SD_MOSI_GPIO GROWBOX_SD_MISO_GPIO GROWBOX_SD_SCLK_GPIO GROWBOX_SD_CS_GPIO)
    if(NOT "${GROWBOX_SD_POWER_GPIO}" STREQUAL "-1")
      list(APPEND _growbox_sd_pins GROWBOX_SD_POWER_GPIO)
    endif()
    foreach(_growbox_sd_pin IN LISTS _growbox_sd_pins)
      foreach(_growbox_i2c_pin IN ITEMS GROWBOX_I2C_SDA_GPIO GROWBOX_I2C_SCL_GPIO)
        growbox_require_distinct_gpio(${_growbox_sd_pin} ${_growbox_i2c_pin}
                                      "SD and I2C enabled together")
      endforeach()
      if(GROWBOX_RF433_LOOPBACK_ENABLED)
        foreach(_growbox_rf_pin IN ITEMS GROWBOX_RF433_TX_GPIO GROWBOX_RF433_RX_GPIO)
          growbox_require_distinct_gpio(${_growbox_sd_pin} ${_growbox_rf_pin}
                                        "SD and RF433 enabled together")
        endforeach()
      endif()
    endforeach()
  endif()

  if(GROWBOX_EINK_DISPLAY_ENABLED)
    set(_growbox_eink_pins
        GROWBOX_EINK_SCLK_GPIO GROWBOX_EINK_MOSI_GPIO GROWBOX_EINK_CS_GPIO GROWBOX_EINK_DC_GPIO
        GROWBOX_EINK_RST_GPIO GROWBOX_EINK_BUSY_GPIO GROWBOX_EINK_POWER_GPIO)
    foreach(_growbox_eink_pin IN LISTS _growbox_eink_pins)
      growbox_require_assigned_gpio(${_growbox_eink_pin} "e-ink display enabled")
      foreach(_growbox_i2c_pin IN ITEMS GROWBOX_I2C_SDA_GPIO GROWBOX_I2C_SCL_GPIO)
        growbox_require_distinct_gpio(${_growbox_eink_pin} ${_growbox_i2c_pin}
                                      "e-ink display and I2C enabled together")
      endforeach()
      if(GROWBOX_RF433_LOOPBACK_ENABLED)
        foreach(_growbox_rf_pin IN ITEMS GROWBOX_RF433_TX_GPIO GROWBOX_RF433_RX_GPIO)
          growbox_require_distinct_gpio(${_growbox_eink_pin} ${_growbox_rf_pin}
                                        "e-ink display and RF433 enabled together")
        endforeach()
      endif()
      if(GROWBOX_STAGE27_SD_ENABLED)
        foreach(_growbox_sd_pin IN LISTS _growbox_sd_pins)
          growbox_require_distinct_gpio(${_growbox_eink_pin} ${_growbox_sd_pin}
                                        "e-ink display and SD enabled together")
        endforeach()
      endif()
    endforeach()

    set(_growbox_eink_key_pins
        GROWBOX_EINK_KEY_HOME_GPIO GROWBOX_EINK_KEY_BACK_GPIO GROWBOX_EINK_KEY_PREVIOUS_GPIO
        GROWBOX_EINK_KEY_NEXT_GPIO GROWBOX_EINK_KEY_OK_GPIO)
    foreach(_growbox_key_pin IN LISTS _growbox_eink_key_pins)
      growbox_require_assigned_gpio(${_growbox_key_pin} "e-ink display buttons enabled")
      foreach(_growbox_eink_pin IN LISTS _growbox_eink_pins)
        growbox_require_distinct_gpio(${_growbox_key_pin} ${_growbox_eink_pin}
                                      "e-ink buttons and display enabled together")
      endforeach()
      foreach(_growbox_i2c_pin IN ITEMS GROWBOX_I2C_SDA_GPIO GROWBOX_I2C_SCL_GPIO)
        growbox_require_distinct_gpio(${_growbox_key_pin} ${_growbox_i2c_pin}
                                      "e-ink buttons and I2C enabled together")
      endforeach()
      if(GROWBOX_RF433_LOOPBACK_ENABLED)
        foreach(_growbox_rf_pin IN ITEMS GROWBOX_RF433_TX_GPIO GROWBOX_RF433_RX_GPIO)
          growbox_require_distinct_gpio(${_growbox_key_pin} ${_growbox_rf_pin}
                                        "e-ink buttons and RF433 enabled together")
        endforeach()
      endif()
      if(GROWBOX_STAGE27_SD_ENABLED)
        foreach(_growbox_sd_pin IN LISTS _growbox_sd_pins)
          growbox_require_distinct_gpio(${_growbox_key_pin} ${_growbox_sd_pin}
                                        "e-ink buttons and SD enabled together")
        endforeach()
      endif()
    endforeach()

    list(LENGTH _growbox_eink_key_pins _growbox_key_pin_count)
    math(EXPR _growbox_key_last_index "${_growbox_key_pin_count} - 1")
    foreach(_growbox_key_left_index RANGE 0 ${_growbox_key_last_index})
      math(EXPR _growbox_key_right_start "${_growbox_key_left_index} + 1")
      if(_growbox_key_right_start LESS _growbox_key_pin_count)
        foreach(_growbox_key_right_index RANGE ${_growbox_key_right_start} ${_growbox_key_last_index})
          list(GET _growbox_eink_key_pins ${_growbox_key_left_index} _growbox_key_left_pin)
          list(GET _growbox_eink_key_pins ${_growbox_key_right_index} _growbox_key_right_pin)
          growbox_require_distinct_gpio(${_growbox_key_left_pin} ${_growbox_key_right_pin}
                                        "e-ink button pins")
        endforeach()
      endif()
    endforeach()

    list(LENGTH _growbox_eink_pins _growbox_eink_pin_count)
    math(EXPR _growbox_eink_last_index "${_growbox_eink_pin_count} - 1")
    foreach(_growbox_eink_left_index RANGE 0 ${_growbox_eink_last_index})
      math(EXPR _growbox_eink_right_start "${_growbox_eink_left_index} + 1")
      if(_growbox_eink_right_start LESS _growbox_eink_pin_count)
        foreach(_growbox_eink_right_index RANGE ${_growbox_eink_right_start} ${_growbox_eink_last_index})
          list(GET _growbox_eink_pins ${_growbox_eink_left_index} _growbox_eink_left_pin)
          list(GET _growbox_eink_pins ${_growbox_eink_right_index} _growbox_eink_right_pin)
          growbox_require_distinct_gpio(${_growbox_eink_left_pin} ${_growbox_eink_right_pin}
                                        "e-ink display pins")
        endforeach()
      endif()
    endforeach()
  endif()
endif()
