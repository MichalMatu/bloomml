#pragma once

#include "climate/display/DisplayMonochromeRaster.h"
#include "climate/display/DisplayRuntime.h"
#include "climate/display/Ssd1680FrameMapper.h"

#include <driver/spi_master.h>

#include <cstdint>

namespace growbox::app::climate_io::display {

struct CrowPanelSsd1680Pins final {
  int sclk{-1};
  int mosi{-1};
  int cs{-1};
  int dc{-1};
  int rst{-1};
  int busy{-1};
  int power{-1};
};

struct CrowPanelSsd1680Config final {
  CrowPanelSsd1680Pins pins{};
  Ssd1680Rotation rotation{Ssd1680Rotation::Clockwise90};
  std::uint32_t spi_clock_hz{10'000'000U};
  std::uint32_t init_busy_timeout_ms{1'500U};
  std::uint32_t partial_busy_timeout_ms{5'000U};
  std::uint32_t full_busy_timeout_ms{15'000U};
};

// Native ESP-IDF backend for the on-board 2.9-inch CrowPanel SSD1680 e-paper.
// It owns one fixed 296x128 1-bpp framebuffer and streams a rotated 128x296
// view directly to controller RAM. No second framebuffer or dynamic application
// allocation is used. The backend is intentionally not a runtime task owner.
class CrowPanelSsd1680DisplayBackend final {
public:
  explicit CrowPanelSsd1680DisplayBackend(CrowPanelSsd1680Config config) noexcept;
  ~CrowPanelSsd1680DisplayBackend() noexcept;

  CrowPanelSsd1680DisplayBackend(const CrowPanelSsd1680DisplayBackend&) = delete;
  CrowPanelSsd1680DisplayBackend& operator=(const CrowPanelSsd1680DisplayBackend&) = delete;

  bool beginFrame(std::uint16_t width_px, std::uint16_t height_px, bool warning,
                  DisplayRefreshKind refresh_kind) noexcept;
  bool drawText(const ClayDisplayTextElement& element) noexcept;
  bool endFrame() noexcept;
  void cancelFrame() noexcept;

  bool hardwareReady() const noexcept { return controller_initialized_; }
  bool previousRamSeeded() const noexcept { return previous_ram_seeded_; }

private:
  static constexpr spi_host_device_t kSpiHost = SPI2_HOST;
  static constexpr std::size_t kTransferChunkBytes = 64U;

  bool configValid() const noexcept;
  bool ensureHardwareReady() noexcept;
  bool initializeGpio() noexcept;
  bool initializeSpi() noexcept;
  bool initializeController() noexcept;
  bool hardwareReset() noexcept;
  bool waitWhileBusy(std::uint32_t timeout_ms) noexcept;
  bool sendCommand(std::uint8_t command) noexcept;
  bool sendData(const std::uint8_t* data, std::size_t length) noexcept;
  bool sendCommandData(std::uint8_t command, const std::uint8_t* data,
                       std::size_t length) noexcept;
  bool setFullRamWindow() noexcept;
  bool setRamCountersToOrigin() noexcept;
  bool writeMappedRam(std::uint8_t command) noexcept;
  bool activate(DisplayRefreshKind kind) noexcept;
  void releaseHardware() noexcept;

  CrowPanelSsd1680Config config_{};
  DisplayMonochromeRaster::Buffer framebuffer_{};
  DisplayMonochromeRaster raster_;
  spi_device_handle_t spi_device_{nullptr};
  DisplayRefreshKind planned_refresh_{DisplayRefreshKind::None};
  bool gpio_initialized_{false};
  bool spi_bus_initialized_{false};
  bool controller_initialized_{false};
  bool previous_ram_seeded_{false};
  bool frame_open_{false};
};

} // namespace growbox::app::climate_io::display
