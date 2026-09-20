#pragma once

#include "climate/display/DisplayDirtyRegion.h"
#include "climate/display/DisplayMonochromeRaster.h"
#include "climate/display/DisplayRuntime.h"
#include "climate/display/Ssd1680FrameMapper.h"

#include <driver/spi_master.h>

#include <cstddef>
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
  bool drawText(const DisplayTextElement& element) noexcept;
  bool setContentRegion(const DisplayRegion& content_region) noexcept;
  bool endFrame() noexcept;
  void cancelFrame() noexcept;

  // Mutable access is available only while a frame transaction is open. The
  // backend remains the sole owner of this storage; external renderers may fill
  // it but cannot replace or retain it.
  std::uint8_t* framebufferData() noexcept {
    return frame_open_ ? framebuffer_.data() : nullptr;
  }

  std::size_t framebufferBytes() const noexcept {
    return framebuffer_.size();
  }

  bool hardwareReady() const noexcept {
    return controller_initialized_;
  }
  bool previousRamSeeded() const noexcept {
    return previous_ram_seeded_;
  }
  const DisplayRegion& lastTransferRegion() const noexcept {
    return last_transfer_region_;
  }
  const Ssd1680NativeWindow& lastNativeWindow() const noexcept {
    return last_native_window_;
  }
  std::size_t lastWindowBytes() const noexcept {
    return last_window_bytes_;
  }
  std::size_t lastRamPayloadBytes() const noexcept {
    return last_ram_payload_bytes_;
  }
  bool lastTransferPartial() const noexcept {
    return last_transfer_partial_;
  }

private:
  static constexpr spi_host_device_t kSpiHost = SPI2_HOST;
  static constexpr std::size_t kTransferChunkBytes = 64U;
  static constexpr std::uint16_t kPartialRefreshPaddingPx = 16U;

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
  static constexpr Ssd1680NativeWindow fullNativeWindow() noexcept {
    return {0U, static_cast<std::uint8_t>(Ssd1680FrameMapper::kNativeBytesPerRow - 1U), 0U,
            static_cast<std::uint16_t>(Ssd1680FrameMapper::kNativeHeightPx - 1U)};
  }
  bool setRamWindow(const Ssd1680NativeWindow& window) noexcept;
  bool setRamCounters(const Ssd1680NativeWindow& window) noexcept;
  bool setFullRamWindow() noexcept;
  bool setRamCountersToOrigin() noexcept;
  bool writeMappedRam(std::uint8_t command, const Ssd1680NativeWindow& window) noexcept;
  bool activate(DisplayRefreshKind kind) noexcept;
  void recordSuccessfulTransfer(const DisplayRegion& transfer_region,
                                const Ssd1680NativeWindow& window,
                                DisplayRefreshKind kind) noexcept;
  void releaseHardware() noexcept;

  CrowPanelSsd1680Config config_{};
  DisplayMonochromeRaster::Buffer framebuffer_{};
  DisplayMonochromeRaster raster_;
  spi_device_handle_t spi_device_{nullptr};
  DisplayDirtyRegionTracker dirty_region_tracker_{};
  DisplayRegion staged_content_region_{};
  DisplayRegion last_transfer_region_{};
  Ssd1680NativeWindow last_native_window_{};
  std::size_t last_window_bytes_{0U};
  std::size_t last_ram_payload_bytes_{0U};
  DisplayRefreshKind planned_refresh_{DisplayRefreshKind::None};
  bool gpio_initialized_{false};
  bool spi_bus_initialized_{false};
  bool controller_initialized_{false};
  bool previous_ram_seeded_{false};
  bool frame_open_{false};
  bool content_region_staged_{false};
  bool last_transfer_partial_{false};
};

} // namespace growbox::app::climate_io::display
