#include "climate/display/CrowPanelDisplayService.h"

#include "climate/display/DisplayClayRenderCoordinator.h"
#include "growbox_clay_ui/PageDirtyRegion.h"
#include "growbox_clay_ui/PageRenderer.h"

#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_timer.h>

#include <limits>

namespace growbox::app::climate_io::display {
namespace {

constexpr char kTag[] = "eink_display";

} // namespace

bool CrowPanelDisplayService::begin() noexcept {
  if (started_) {
    return true;
  }

  clay_arena_required_bytes_ = ::growbox::clay_ui::pageRendererArenaBytes();
  if (clay_arena_required_bytes_ == 0U ||
      clay_arena_required_bytes_ > std::numeric_limits<std::uint32_t>::max()) {
    ESP_LOGE(kTag, "Invalid Clay arena requirement bytes=%llu",
             static_cast<unsigned long long>(clay_arena_required_bytes_));
    return false;
  }

  clay_arena_ = static_cast<std::uint8_t*>(
      heap_caps_malloc(clay_arena_required_bytes_, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (clay_arena_ == nullptr) {
    ESP_LOGE(kTag, "Failed to allocate Clay arena in PSRAM bytes=%lu",
             static_cast<unsigned long>(clay_arena_required_bytes_));
    return false;
  }
  clay_arena_bytes_ = clay_arena_required_bytes_;

  render_queue_ = xQueueCreateStatic(1U, sizeof(DisplayRenderWorkItem),
                                     render_queue_storage_.data(), &render_queue_control_);
  completion_queue_ =
      xQueueCreateStatic(1U, sizeof(DisplayRenderCompletion), completion_queue_storage_.data(),
                         &completion_queue_control_);
  if (render_queue_ == nullptr || completion_queue_ == nullptr) {
    render_queue_ = nullptr;
    completion_queue_ = nullptr;
    releaseClayArena();
    ESP_LOGE(kTag, "Failed to create static display queues");
    return false;
  }

  stack_min_free_bytes_.store(taskStackBytes(), std::memory_order_relaxed);
  task_ =
      xTaskCreateStatic(&CrowPanelDisplayService::taskEntry, "eink_display", taskStackBytes(), this,
                        tskIDLE_PRIORITY + 1U, task_stack_storage_.data(), &task_control_);
  if (task_ == nullptr) {
    vQueueDelete(render_queue_);
    vQueueDelete(completion_queue_);
    render_queue_ = nullptr;
    completion_queue_ = nullptr;
    releaseClayArena();
    ESP_LOGE(kTag, "Failed to create static display worker task");
    return false;
  }

  started_ = true;
  ESP_LOGI(kTag, "Display worker started static_stack_bytes=%u clay_arena_psram_bytes=%lu",
           static_cast<unsigned>(taskStackBytes()), static_cast<unsigned long>(clay_arena_bytes_));
  return true;
}

void CrowPanelDisplayService::tick(std::uint64_t now_ms) noexcept {
  if (!started_) {
    return;
  }
  drainCompletions(now_ms);
  submitPending(now_ms);
}

CrowPanelDisplayServiceStatus CrowPanelDisplayService::status() const noexcept {
  CrowPanelDisplayServiceStatus result{};
  result.started = started_;
  result.render_in_flight = transaction_.inFlight();
  result.clay_arena_in_psram = clay_arena_ != nullptr;
  result.clay_arena_required_bytes = static_cast<std::uint32_t>(clay_arena_required_bytes_);
  result.clay_arena_allocated_bytes = static_cast<std::uint32_t>(clay_arena_bytes_);
  result.submitted = submitted_.load(std::memory_order_relaxed);
  result.submit_failures = submit_failures_.load(std::memory_order_relaxed);
  result.render_successes = render_successes_.load(std::memory_order_relaxed);
  result.render_failures = render_failures_.load(std::memory_order_relaxed);
  result.confirm_failures = confirm_failures_.load(std::memory_order_relaxed);
  result.stale_completions = stale_completions_.load(std::memory_order_relaxed);
  result.stack_min_free_bytes = stack_min_free_bytes_.load(std::memory_order_relaxed);
  return result;
}

void CrowPanelDisplayService::taskEntry(void* context) noexcept {
  static_cast<CrowPanelDisplayService*>(context)->taskLoop();
}

void CrowPanelDisplayService::observeStackWatermark() noexcept {
  stack_min_free_bytes_.store(static_cast<std::uint32_t>(uxTaskGetStackHighWaterMark(nullptr)),
                              std::memory_order_relaxed);
}

void CrowPanelDisplayService::taskLoop() noexcept {
  DisplayRenderWorkItem work_item{};
  while (true) {
    if (xQueueReceive(render_queue_, &work_item, portMAX_DELAY) != pdTRUE) {
      continue;
    }

    DisplayRegion transfer_region{0U, 0U, ::growbox::clay_ui::kDisplayWidth,
                                  ::growbox::clay_ui::kDisplayHeight};
    if (work_item.frame.refresh_kind == DisplayRefreshKind::Partial &&
        has_last_physical_page_model_) {
      ::growbox::clay_ui::PageDirtyRegion dirty{};
      if (::growbox::clay_ui::planPageDirtyRegion(&last_physical_page_model_,
                                                  work_item.frame.page_model, dirty)) {
        transfer_region = {dirty.x_px, dirty.y_px, dirty.width_px, dirty.height_px};
      }
    }

    ::growbox::clay_ui::RenderSummary render_summary{};
    const bool success = renderClayDisplayFrame(work_item.frame, transfer_region, clay_arena_,
                                                clay_arena_bytes_, backend_, &render_summary);
    observeStackWatermark();
    const std::uint32_t stack_min_free_bytes =
        stack_min_free_bytes_.load(std::memory_order_relaxed);
    if (success) {
      last_physical_page_model_ = work_item.frame.page_model;
      has_last_physical_page_model_ = true;
      const std::uint32_t successes =
          render_successes_.fetch_add(1U, std::memory_order_relaxed) + 1U;
      const DisplayRegion transfer_region = backend_.lastTransferRegion();
      const Ssd1680NativeWindow native_window = backend_.lastNativeWindow();
      ESP_LOGI(kTag,
               "Physical Clay refresh completed generation=%llu kind=%u reason=%u successes=%lu "
               "commands=%lu black_pixels=%lu dirty=%u,%u,%u,%u native=%u-%u,%u-%u "
               "window_bytes=%lu ram_payload_bytes=%lu physical_partial=%u "
               "stack_min_free_bytes=%lu",
               static_cast<unsigned long long>(work_item.generation),
               static_cast<unsigned>(work_item.frame.refresh_kind),
               static_cast<unsigned>(work_item.frame.refresh_reason),
               static_cast<unsigned long>(successes),
               static_cast<unsigned long>(render_summary.render_commands),
               static_cast<unsigned long>(render_summary.black_pixels),
               static_cast<unsigned>(transfer_region.x_px),
               static_cast<unsigned>(transfer_region.y_px),
               static_cast<unsigned>(transfer_region.width_px),
               static_cast<unsigned>(transfer_region.height_px),
               static_cast<unsigned>(native_window.x_start_byte),
               static_cast<unsigned>(native_window.x_end_byte),
               static_cast<unsigned>(native_window.y_start_px),
               static_cast<unsigned>(native_window.y_end_px),
               static_cast<unsigned long>(backend_.lastWindowBytes()),
               static_cast<unsigned long>(backend_.lastRamPayloadBytes()),
               backend_.lastTransferPartial() ? 1U : 0U,
               static_cast<unsigned long>(stack_min_free_bytes));
    } else {
      const std::uint32_t failures = render_failures_.fetch_add(1U, std::memory_order_relaxed) + 1U;
      ESP_LOGE(kTag,
               "Physical Clay refresh failed generation=%llu kind=%u reason=%u failures=%lu "
               "stack_min_free_bytes=%lu",
               static_cast<unsigned long long>(work_item.generation),
               static_cast<unsigned>(work_item.frame.refresh_kind),
               static_cast<unsigned>(work_item.frame.refresh_reason),
               static_cast<unsigned long>(failures),
               static_cast<unsigned long>(stack_min_free_bytes));
    }

    DisplayRenderCompletion completion{};
    completion.generation = work_item.generation;
    completion.rendered_at_ms = static_cast<std::uint64_t>(esp_timer_get_time()) / 1000U;
    completion.success = success;
    (void)xQueueOverwrite(completion_queue_, &completion);
  }
}

void CrowPanelDisplayService::drainCompletions(std::uint64_t now_ms) noexcept {
  DisplayRenderCompletion completion{};
  while (xQueueReceive(completion_queue_, &completion, 0U) == pdTRUE) {
    if (!transaction_.matches(completion)) {
      stale_completions_.fetch_add(1U, std::memory_order_relaxed);
      ESP_LOGW(kTag, "Ignoring stale completion generation=%llu",
               static_cast<unsigned long long>(completion.generation));
      continue;
    }

    if (completion.success &&
        !observer_.confirmRendered(transaction_.inFlightFrame(), completion.rendered_at_ms)) {
      confirm_failures_.fetch_add(1U, std::memory_order_relaxed);
      ESP_LOGE(kTag, "Display render confirmation failed generation=%llu",
               static_cast<unsigned long long>(completion.generation));
    }
    if (!completion.success) {
      next_submit_ms_ = now_ms + retry_backoff_ms_;
    }
    (void)transaction_.finish(completion);
  }
}

void CrowPanelDisplayService::submitPending(std::uint64_t now_ms) noexcept {
  if (transaction_.inFlight() || now_ms < next_submit_ms_ || !observer_.hasFrame() ||
      !observer_.hasPendingRefresh()) {
    return;
  }

  DisplayRenderWorkItem work_item{};
  if (!transaction_.start(observer_.lastFrame(), work_item)) {
    return;
  }
  if (xQueueSend(render_queue_, &work_item, 0U) != pdTRUE) {
    transaction_.abort();
    const std::uint32_t failures = submit_failures_.fetch_add(1U, std::memory_order_relaxed) + 1U;
    ESP_LOGW(kTag, "Display queue submit failed failures=%lu",
             static_cast<unsigned long>(failures));
    return;
  }
  submitted_.fetch_add(1U, std::memory_order_relaxed);
}

void CrowPanelDisplayService::releaseClayArena() noexcept {
  if (clay_arena_ != nullptr) {
    heap_caps_free(clay_arena_);
  }
  clay_arena_ = nullptr;
  clay_arena_bytes_ = 0U;
}

} // namespace growbox::app::climate_io::display
