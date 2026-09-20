#pragma once

#include "climate/display/CrowPanelSsd1680DisplayBackend.h"
#include "climate/display/DisplayAsyncTransaction.h"
#include "climate/display/DisplayTelemetryObserver.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace growbox::app::climate_io::display {

struct CrowPanelDisplayServiceStatus final {
  bool started{false};
  bool render_in_flight{false};
  bool clay_arena_in_psram{false};
  std::uint32_t clay_arena_required_bytes{0U};
  std::uint32_t clay_arena_allocated_bytes{0U};
  std::uint32_t submitted{0U};
  std::uint32_t submit_failures{0U};
  std::uint32_t render_successes{0U};
  std::uint32_t render_failures{0U};
  std::uint32_t confirm_failures{0U};
  std::uint32_t stale_completions{0U};
  std::uint32_t stack_min_free_bytes{0U};
};

// Low-priority asynchronous owner of the physical e-paper backend. tick() is
// called only from the main runtime owner and performs non-blocking queue work;
// all potentially multi-second SSD1680 BUSY waits stay on the worker task.
class CrowPanelDisplayService final {
public:
  static constexpr std::uint32_t kTaskStackBytes = 6'144U;

  CrowPanelDisplayService(DisplayTelemetryObserver& observer,
                          CrowPanelSsd1680DisplayBackend& backend,
                          std::uint64_t retry_backoff_ms = 30'000U) noexcept
      : observer_(observer), backend_(backend), retry_backoff_ms_(retry_backoff_ms) {}

  CrowPanelDisplayService(const CrowPanelDisplayService&) = delete;
  CrowPanelDisplayService& operator=(const CrowPanelDisplayService&) = delete;

  bool begin() noexcept;
  void tick(std::uint64_t now_ms) noexcept;
  CrowPanelDisplayServiceStatus status() const noexcept;

  static constexpr std::uint32_t taskStackBytes() noexcept {
    return kTaskStackBytes;
  }

private:
  static constexpr std::size_t kTaskStackElements =
      (kTaskStackBytes + sizeof(StackType_t) - 1U) / sizeof(StackType_t);

  static void taskEntry(void* context) noexcept;
  void taskLoop() noexcept;
  void observeStackWatermark() noexcept;
  void drainCompletions(std::uint64_t now_ms) noexcept;
  void submitPending(std::uint64_t now_ms) noexcept;
  void releaseClayArena() noexcept;

  static_assert(std::is_trivially_copyable_v<DisplayRenderWorkItem>);
  static_assert(std::is_trivially_copyable_v<DisplayRenderCompletion>);

  DisplayTelemetryObserver& observer_;
  CrowPanelSsd1680DisplayBackend& backend_;
  std::uint64_t retry_backoff_ms_{30'000U};
  std::uint64_t next_submit_ms_{0U};
  DisplayAsyncTransaction transaction_{};

  StaticQueue_t render_queue_control_{};
  StaticQueue_t completion_queue_control_{};
  std::array<std::uint8_t, sizeof(DisplayRenderWorkItem)> render_queue_storage_{};
  std::array<std::uint8_t, sizeof(DisplayRenderCompletion)> completion_queue_storage_{};
  StaticTask_t task_control_{};
  std::array<StackType_t, kTaskStackElements> task_stack_storage_{};
  QueueHandle_t render_queue_{nullptr};
  QueueHandle_t completion_queue_{nullptr};
  TaskHandle_t task_{nullptr};
  std::uint8_t* clay_arena_{nullptr};
  std::size_t clay_arena_required_bytes_{0U};
  std::size_t clay_arena_bytes_{0U};
  bool started_{false};

  std::atomic<std::uint32_t> submitted_{0U};
  std::atomic<std::uint32_t> submit_failures_{0U};
  std::atomic<std::uint32_t> render_successes_{0U};
  std::atomic<std::uint32_t> render_failures_{0U};
  std::atomic<std::uint32_t> confirm_failures_{0U};
  std::atomic<std::uint32_t> stale_completions_{0U};
  std::atomic<std::uint32_t> stack_min_free_bytes_{kTaskStackBytes};
};

} // namespace growbox::app::climate_io::display
