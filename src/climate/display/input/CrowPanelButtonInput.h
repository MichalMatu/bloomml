#pragma once

#include "climate/display/input/DisplayButtonStateMachine.h"

#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace growbox::app::climate_io::display {

struct CrowPanelButtonPins final {
  int home{-1};
  int back{-1};
  int previous{-1};
  int next{-1};
  int ok{-1};
};

struct CrowPanelButtonInputConfig final {
  CrowPanelButtonPins pins{};
  DisplayButtonTiming timing{};
  std::uint32_t sample_period_ms{20U};
};

struct CrowPanelButtonInputStatus final {
  bool started{false};
  std::uint32_t events_emitted{0U};
  std::uint32_t long_presses{0U};
  std::uint32_t queue_drops{0U};
};

// Native ESP-IDF GPIO sampler. The esp_timer callback only reads five GPIOs,
// advances pure per-key state machines, and appends semantic events to a static
// queue. Navigation stays in DisplayRuntimeController and is never mutated here.
class CrowPanelButtonInput final {
public:
  explicit CrowPanelButtonInput(CrowPanelButtonInputConfig config) noexcept;
  ~CrowPanelButtonInput() noexcept;

  CrowPanelButtonInput(const CrowPanelButtonInput&) = delete;
  CrowPanelButtonInput& operator=(const CrowPanelButtonInput&) = delete;

  bool begin() noexcept;
  bool poll(DisplayButtonEvent& event) noexcept;
  CrowPanelButtonInputStatus status() const noexcept;

private:
  static constexpr std::size_t kButtonCount = 5U;
  static constexpr UBaseType_t kQueueDepth = 8U;

  static void timerEntry(void* context) noexcept;
  void sample() noexcept;
  bool configValid() const noexcept;
  void stop() noexcept;

  static_assert(std::is_trivially_copyable_v<DisplayButtonEvent>);

  CrowPanelButtonInputConfig config_{};
  std::array<int, kButtonCount> pins_{};
  std::array<DisplayButton, kButtonCount> buttons_{};
  std::array<DisplayButtonStateMachine, kButtonCount> states_{};

  StaticQueue_t queue_control_{};
  std::array<std::uint8_t, kQueueDepth * sizeof(DisplayButtonEvent)> queue_storage_{};
  QueueHandle_t queue_{nullptr};
  esp_timer_handle_t timer_{nullptr};
  std::atomic<bool> started_{false};

  std::atomic<std::uint32_t> events_emitted_{0U};
  std::atomic<std::uint32_t> long_presses_{0U};
  std::atomic<std::uint32_t> queue_drops_{0U};
};

} // namespace growbox::app::climate_io::display
