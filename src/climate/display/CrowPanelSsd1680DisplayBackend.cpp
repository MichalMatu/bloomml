#include "climate/display/CrowPanelSsd1680DisplayBackend.h"

#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <array>
#include <cstddef>
#include <cstdint>

namespace growbox::app::climate_io::display {
namespace {

constexpr std::uint8_t kCmdSwReset = 0x12U;
constexpr std::uint8_t kCmdDriverOutputControl = 0x01U;
constexpr std::uint8_t kCmdDataEntryMode = 0x11U;
constexpr std::uint8_t kCmdBorderWaveform = 0x3CU;
constexpr std::uint8_t kCmdDisplayUpdateControl1 = 0x21U;
constexpr std::uint8_t kCmdTemperatureSensorControl = 0x18U;
constexpr std::uint8_t kCmdRamXWindow = 0x44U;
constexpr std::uint8_t kCmdRamYWindow = 0x45U;
constexpr std::uint8_t kCmdRamXCounter = 0x4EU;
constexpr std::uint8_t kCmdRamYCounter = 0x4FU;
constexpr std::uint8_t kCmdWriteCurrentRam = 0x24U;
constexpr std::uint8_t kCmdWritePreviousRam = 0x26U;
constexpr std::uint8_t kCmdDisplayUpdateControl2 = 0x22U;
constexpr std::uint8_t kCmdMasterActivation = 0x20U;
constexpr std::uint8_t kFullUpdate = 0xF7U;
constexpr std::uint8_t kPartialUpdate = 0xFCU;
constexpr std::uint32_t kPanelPowerDelayMs = 10U;
constexpr std::uint32_t kResetPulseMs = 10U;
constexpr std::uint32_t kBusyPollMs = 10U;
constexpr std::uint32_t kMaxSpiClockHz = 20'000'000U;

void delayMs(std::uint32_t milliseconds) noexcept {
  vTaskDelay(pdMS_TO_TICKS(milliseconds));
}

} // namespace

CrowPanelSsd1680DisplayBackend::CrowPanelSsd1680DisplayBackend(
    CrowPanelSsd1680Config config) noexcept
    : config_(config), raster_(framebuffer_) {
  raster_.clearWhite();
}

CrowPanelSsd1680DisplayBackend::~CrowPanelSsd1680DisplayBackend() noexcept {
  releaseHardware();
}

bool CrowPanelSsd1680DisplayBackend::beginFrame(std::uint16_t width_px, std::uint16_t height_px,
                                                bool warning,
                                                DisplayRefreshKind refresh_kind) noexcept {
  (void)warning;
  if (frame_open_ || width_px != DisplayMonochromeRaster::kWidthPx ||
      height_px != DisplayMonochromeRaster::kHeightPx || refresh_kind == DisplayRefreshKind::None ||
      !configValid()) {
    return false;
  }

  raster_.clearWhite();
  staged_transfer_region_ = {};
  transfer_region_staged_ = false;
  planned_refresh_ = refresh_kind;
  frame_open_ = true;
  return true;
}

bool CrowPanelSsd1680DisplayBackend::drawText(const DisplayTextElement& element) noexcept {
  return frame_open_ && raster_.drawText(element);
}

bool CrowPanelSsd1680DisplayBackend::setTransferRegion(
    const DisplayRegion& transfer_region) noexcept {
  if (!frame_open_) {
    return false;
  }
  staged_transfer_region_ = clampDisplayRegion(transfer_region, DisplayMonochromeRaster::kWidthPx,
                                               DisplayMonochromeRaster::kHeightPx);
  transfer_region_staged_ = !displayRegionEmpty(staged_transfer_region_);
  return transfer_region_staged_;
}

bool CrowPanelSsd1680DisplayBackend::endFrame() noexcept {
  if (!frame_open_ || planned_refresh_ == DisplayRefreshKind::None || !transfer_region_staged_) {
    return false;
  }

  if (!ensureHardwareReady()) {
    releaseHardware();
    return false;
  }

  const DisplayRefreshKind effective_refresh =
      planned_refresh_ == DisplayRefreshKind::Full || !previous_ram_seeded_
          ? DisplayRefreshKind::Full
          : DisplayRefreshKind::Partial;

  DisplayRegion transfer_region{};
  Ssd1680NativeWindow native_window{};

  if (effective_refresh == DisplayRefreshKind::Full) {
    transfer_region = {0U, 0U, DisplayMonochromeRaster::kWidthPx,
                       DisplayMonochromeRaster::kHeightPx};
    native_window = fullNativeWindow();
    if (!writeMappedRam(kCmdWritePreviousRam, native_window) ||
        !writeMappedRam(kCmdWriteCurrentRam, native_window) ||
        !activate(DisplayRefreshKind::Full)) {
      releaseHardware();
      return false;
    }
    previous_ram_seeded_ = true;
  } else {
    transfer_region =
        expandDisplayRegion(staged_transfer_region_, kPartialRefreshPaddingPx,
                            DisplayMonochromeRaster::kWidthPx, DisplayMonochromeRaster::kHeightPx);
    const bool partial_ok = Ssd1680FrameMapper::nativeWindowForLogicalRegion(
                                transfer_region, config_.rotation, native_window) &&
                            writeMappedRam(kCmdWriteCurrentRam, native_window) &&
                            activate(DisplayRefreshKind::Partial) &&
                            writeMappedRam(kCmdWritePreviousRam, native_window);
    if (!partial_ok) {
      releaseHardware();
      if (!ensureHardwareReady()) {
        releaseHardware();
        return false;
      }

      transfer_region = {0U, 0U, DisplayMonochromeRaster::kWidthPx,
                         DisplayMonochromeRaster::kHeightPx};
      native_window = fullNativeWindow();
      if (!writeMappedRam(kCmdWritePreviousRam, native_window) ||
          !writeMappedRam(kCmdWriteCurrentRam, native_window) ||
          !activate(DisplayRefreshKind::Full)) {
        releaseHardware();
        return false;
      }
      previous_ram_seeded_ = true;
      recordSuccessfulTransfer(transfer_region, native_window, DisplayRefreshKind::Full);
      frame_open_ = false;
      transfer_region_staged_ = false;
      planned_refresh_ = DisplayRefreshKind::None;
      return true;
    }
  }

  recordSuccessfulTransfer(transfer_region, native_window, effective_refresh);
  frame_open_ = false;
  transfer_region_staged_ = false;
  planned_refresh_ = DisplayRefreshKind::None;
  return true;
}

void CrowPanelSsd1680DisplayBackend::cancelFrame() noexcept {
  frame_open_ = false;
  transfer_region_staged_ = false;
  staged_transfer_region_ = {};
  planned_refresh_ = DisplayRefreshKind::None;
}

bool CrowPanelSsd1680DisplayBackend::configValid() const noexcept {
  const std::array<int, 7U> pins{config_.pins.sclk, config_.pins.mosi, config_.pins.cs,
                                 config_.pins.dc,   config_.pins.rst,  config_.pins.busy,
                                 config_.pins.power};
  if (config_.spi_clock_hz == 0U || config_.spi_clock_hz > kMaxSpiClockHz ||
      config_.init_busy_timeout_ms == 0U || config_.partial_busy_timeout_ms == 0U ||
      config_.full_busy_timeout_ms == 0U) {
    return false;
  }
  for (std::size_t left = 0U; left < pins.size(); ++left) {
    if (pins[left] < 0) {
      return false;
    }
    for (std::size_t right = left + 1U; right < pins.size(); ++right) {
      if (pins[left] == pins[right]) {
        return false;
      }
    }
  }
  return true;
}

bool CrowPanelSsd1680DisplayBackend::ensureHardwareReady() noexcept {
  if (controller_initialized_) {
    return true;
  }
  if (!configValid() || !initializeGpio() || !initializeSpi() || !initializeController()) {
    return false;
  }
  controller_initialized_ = true;
  previous_ram_seeded_ = false;
  return true;
}

bool CrowPanelSsd1680DisplayBackend::initializeGpio() noexcept {
  if (gpio_initialized_) {
    return true;
  }

  const auto dc = static_cast<gpio_num_t>(config_.pins.dc);
  const auto rst = static_cast<gpio_num_t>(config_.pins.rst);
  const auto busy = static_cast<gpio_num_t>(config_.pins.busy);
  const auto power = static_cast<gpio_num_t>(config_.pins.power);
  if (gpio_set_direction(dc, GPIO_MODE_OUTPUT) != ESP_OK ||
      gpio_set_direction(rst, GPIO_MODE_OUTPUT) != ESP_OK ||
      gpio_set_direction(power, GPIO_MODE_OUTPUT) != ESP_OK ||
      gpio_set_direction(busy, GPIO_MODE_INPUT) != ESP_OK || gpio_set_level(dc, 0) != ESP_OK ||
      gpio_set_level(rst, 1) != ESP_OK || gpio_set_level(power, 0) != ESP_OK) {
    return false;
  }
  gpio_initialized_ = true;
  return true;
}

bool CrowPanelSsd1680DisplayBackend::initializeSpi() noexcept {
  if (spi_device_ != nullptr) {
    return true;
  }

  if (!spi_bus_initialized_) {
    spi_bus_config_t bus_config{};
    bus_config.mosi_io_num = config_.pins.mosi;
    bus_config.miso_io_num = -1;
    bus_config.sclk_io_num = config_.pins.sclk;
    bus_config.quadwp_io_num = -1;
    bus_config.quadhd_io_num = -1;
    bus_config.max_transfer_sz = static_cast<int>(kTransferChunkBytes);
    if (spi_bus_initialize(kSpiHost, &bus_config, SPI_DMA_DISABLED) != ESP_OK) {
      return false;
    }
    spi_bus_initialized_ = true;
  }

  spi_device_interface_config_t device_config{};
  device_config.clock_speed_hz = static_cast<int>(config_.spi_clock_hz);
  device_config.mode = 0;
  device_config.spics_io_num = config_.pins.cs;
  device_config.queue_size = 1;
  if (spi_bus_add_device(kSpiHost, &device_config, &spi_device_) != ESP_OK) {
    return false;
  }
  return true;
}

bool CrowPanelSsd1680DisplayBackend::initializeController() noexcept {
  if (gpio_set_level(static_cast<gpio_num_t>(config_.pins.power), 1) != ESP_OK) {
    return false;
  }
  delayMs(kPanelPowerDelayMs);
  if (!hardwareReset() || !sendCommand(kCmdSwReset) ||
      !waitWhileBusy(config_.init_busy_timeout_ms)) {
    return false;
  }

  constexpr std::array<std::uint8_t, 3U> driver_output{0x27U, 0x01U, 0x00U};
  constexpr std::array<std::uint8_t, 1U> data_entry{0x03U};
  constexpr std::array<std::uint8_t, 1U> border{0x05U};
  constexpr std::array<std::uint8_t, 2U> update_control_1{0x00U, 0x80U};
  constexpr std::array<std::uint8_t, 1U> temperature_sensor{0x80U};

  return sendCommandData(kCmdDriverOutputControl, driver_output.data(), driver_output.size()) &&
         sendCommandData(kCmdDataEntryMode, data_entry.data(), data_entry.size()) &&
         sendCommandData(kCmdBorderWaveform, border.data(), border.size()) &&
         sendCommandData(kCmdDisplayUpdateControl1, update_control_1.data(),
                         update_control_1.size()) &&
         sendCommandData(kCmdTemperatureSensorControl, temperature_sensor.data(),
                         temperature_sensor.size()) &&
         setFullRamWindow() && setRamCountersToOrigin();
}

bool CrowPanelSsd1680DisplayBackend::hardwareReset() noexcept {
  const auto reset = static_cast<gpio_num_t>(config_.pins.rst);
  if (gpio_set_level(reset, 1) != ESP_OK) {
    return false;
  }
  delayMs(kResetPulseMs);
  if (gpio_set_level(reset, 0) != ESP_OK) {
    return false;
  }
  delayMs(kResetPulseMs);
  if (gpio_set_level(reset, 1) != ESP_OK) {
    return false;
  }
  delayMs(kResetPulseMs);
  return waitWhileBusy(config_.init_busy_timeout_ms);
}

bool CrowPanelSsd1680DisplayBackend::waitWhileBusy(std::uint32_t timeout_ms) noexcept {
  const std::int64_t started_us = esp_timer_get_time();
  const std::int64_t timeout_us = static_cast<std::int64_t>(timeout_ms) * 1'000LL;
  const auto busy = static_cast<gpio_num_t>(config_.pins.busy);
  while (gpio_get_level(busy) != 0) {
    if (esp_timer_get_time() - started_us >= timeout_us) {
      return false;
    }
    delayMs(kBusyPollMs);
  }
  return true;
}

bool CrowPanelSsd1680DisplayBackend::sendCommand(std::uint8_t command) noexcept {
  if (spi_device_ == nullptr ||
      gpio_set_level(static_cast<gpio_num_t>(config_.pins.dc), 0) != ESP_OK) {
    return false;
  }
  spi_transaction_t transaction{};
  transaction.length = 8U;
  transaction.tx_buffer = &command;
  return spi_device_transmit(spi_device_, &transaction) == ESP_OK;
}

bool CrowPanelSsd1680DisplayBackend::sendData(const std::uint8_t* data,
                                              std::size_t length) noexcept {
  if (spi_device_ == nullptr || data == nullptr || length == 0U || length > kTransferChunkBytes ||
      gpio_set_level(static_cast<gpio_num_t>(config_.pins.dc), 1) != ESP_OK) {
    return false;
  }
  spi_transaction_t transaction{};
  transaction.length = length * 8U;
  transaction.tx_buffer = data;
  return spi_device_transmit(spi_device_, &transaction) == ESP_OK;
}

bool CrowPanelSsd1680DisplayBackend::sendCommandData(std::uint8_t command, const std::uint8_t* data,
                                                     std::size_t length) noexcept {
  return sendCommand(command) && sendData(data, length);
}

bool CrowPanelSsd1680DisplayBackend::setRamWindow(const Ssd1680NativeWindow& window) noexcept {
  if (window.x_end_byte < window.x_start_byte ||
      window.x_end_byte >= Ssd1680FrameMapper::kNativeBytesPerRow ||
      window.y_end_px < window.y_start_px ||
      window.y_end_px >= Ssd1680FrameMapper::kNativeHeightPx) {
    return false;
  }

  const std::array<std::uint8_t, 2U> x_window{window.x_start_byte, window.x_end_byte};
  const std::array<std::uint8_t, 4U> y_window{
      static_cast<std::uint8_t>(window.y_start_px & 0xFFU),
      static_cast<std::uint8_t>((window.y_start_px >> 8U) & 0xFFU),
      static_cast<std::uint8_t>(window.y_end_px & 0xFFU),
      static_cast<std::uint8_t>((window.y_end_px >> 8U) & 0xFFU),
  };
  return sendCommandData(kCmdRamXWindow, x_window.data(), x_window.size()) &&
         sendCommandData(kCmdRamYWindow, y_window.data(), y_window.size());
}

bool CrowPanelSsd1680DisplayBackend::setRamCounters(const Ssd1680NativeWindow& window) noexcept {
  const std::array<std::uint8_t, 1U> x_counter{window.x_start_byte};
  const std::array<std::uint8_t, 2U> y_counter{
      static_cast<std::uint8_t>(window.y_start_px & 0xFFU),
      static_cast<std::uint8_t>((window.y_start_px >> 8U) & 0xFFU),
  };
  return sendCommandData(kCmdRamXCounter, x_counter.data(), x_counter.size()) &&
         sendCommandData(kCmdRamYCounter, y_counter.data(), y_counter.size());
}

bool CrowPanelSsd1680DisplayBackend::setFullRamWindow() noexcept {
  return setRamWindow(fullNativeWindow());
}

bool CrowPanelSsd1680DisplayBackend::setRamCountersToOrigin() noexcept {
  return setRamCounters(fullNativeWindow());
}

bool CrowPanelSsd1680DisplayBackend::writeMappedRam(std::uint8_t command,
                                                    const Ssd1680NativeWindow& window) noexcept {
  if (!setRamWindow(window) || !setRamCounters(window) || !sendCommand(command)) {
    return false;
  }

  std::array<std::uint8_t, kTransferChunkBytes> chunk{};
  std::size_t chunk_size = 0U;
  for (std::uint16_t native_y = window.y_start_px; native_y <= window.y_end_px; ++native_y) {
    for (std::uint16_t native_byte_x = window.x_start_byte; native_byte_x <= window.x_end_byte;
         ++native_byte_x) {
      const std::size_t native_index =
          static_cast<std::size_t>(native_y) * Ssd1680FrameMapper::kNativeBytesPerRow +
          native_byte_x;
      if (!Ssd1680FrameMapper::nativeByteAt(framebuffer_, native_index, config_.rotation,
                                            chunk[chunk_size])) {
        return false;
      }
      ++chunk_size;
      if (chunk_size == chunk.size()) {
        if (!sendData(chunk.data(), chunk_size)) {
          return false;
        }
        chunk_size = 0U;
      }
    }
  }

  return chunk_size == 0U || sendData(chunk.data(), chunk_size);
}

bool CrowPanelSsd1680DisplayBackend::activate(DisplayRefreshKind kind) noexcept {
  const std::uint8_t update = kind == DisplayRefreshKind::Full ? kFullUpdate : kPartialUpdate;
  const std::uint32_t timeout = kind == DisplayRefreshKind::Full ? config_.full_busy_timeout_ms
                                                                 : config_.partial_busy_timeout_ms;
  return sendCommandData(kCmdDisplayUpdateControl2, &update, 1U) &&
         sendCommand(kCmdMasterActivation) && waitWhileBusy(timeout);
}

void CrowPanelSsd1680DisplayBackend::recordSuccessfulTransfer(const DisplayRegion& transfer_region,
                                                              const Ssd1680NativeWindow& window,
                                                              DisplayRefreshKind kind) noexcept {
  last_transfer_region_ = transfer_region;
  last_native_window_ = window;
  last_window_bytes_ = displayRegionEmpty(transfer_region) ? 0U : window.transferBytes();
  last_ram_payload_bytes_ = last_window_bytes_ * 2U;
  last_transfer_partial_ = kind == DisplayRefreshKind::Partial;
}

void CrowPanelSsd1680DisplayBackend::releaseHardware() noexcept {
  controller_initialized_ = false;
  previous_ram_seeded_ = false;
  staged_transfer_region_ = {};
  transfer_region_staged_ = false;
  last_transfer_region_ = {};
  last_native_window_ = {};
  last_window_bytes_ = 0U;
  last_ram_payload_bytes_ = 0U;
  last_transfer_partial_ = false;
  if (spi_device_ != nullptr) {
    (void)spi_bus_remove_device(spi_device_);
    spi_device_ = nullptr;
  }
  if (spi_bus_initialized_) {
    (void)spi_bus_free(kSpiHost);
    spi_bus_initialized_ = false;
  }
  if (gpio_initialized_) {
    (void)gpio_set_level(static_cast<gpio_num_t>(config_.pins.power), 0);
  }
}

} // namespace growbox::app::climate_io::display
