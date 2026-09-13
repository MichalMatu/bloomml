#include "climate/display/CrowPanelDisplayService.h"

#include "climate/display/DisplayClayCoordinator.h"

#include <esp_log.h>
#include <esp_timer.h>

namespace growbox::app::climate_io::display {
namespace {

constexpr char kTag[] = "eink_display";

} // namespace

bool CrowPanelDisplayService::begin() noexcept {
  if (started_) {
    return true;
  }

  render_queue_ = xQueueCreateStatic(1U, sizeof(DisplayRenderWorkItem), render_queue_storage_.data(),
                                     &render_queue_control_);
  completion_queue_ =
      xQueueCreateStatic(1U, sizeof(DisplayRenderCompletion), completion_queue_storage_.data(),
                         &completion_queue_control_);
  if (render_queue_ == nullptr || completion_queue_ == nullptr) {
    render_queue_ = nullptr;
    completion_queue_ = nullptr;
    ESP_LOGE(kTag, "Failed to create static display queues");
    return false;
  }

  if (xTaskCreate(&CrowPanelDisplayService::taskEntry, "eink_display", taskStackBytes(), this,
                  tskIDLE_PRIORITY + 1U, &task_) != pdPASS) {
    vQueueDelete(render_queue_);
    vQueueDelete(completion_queue_);
    render_queue_ = nullptr;
    completion_queue_ = nullptr;
    task_ = nullptr;
    ESP_LOGE(kTag, "Failed to create display worker task");
    return false;
  }

  started_ = true;
  ESP_LOGI(kTag, "Display worker started stack_bytes=%u",
           static_cast<unsigned>(taskStackBytes()));
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
  result.submitted = submitted_.load(std::memory_order_relaxed);
  result.submit_failures = submit_failures_.load(std::memory_order_relaxed);
  result.render_successes = render_successes_.load(std::memory_order_relaxed);
  result.render_failures = render_failures_.load(std::memory_order_relaxed);
  result.confirm_failures = confirm_failures_.load(std::memory_order_relaxed);
  result.stale_completions = stale_completions_.load(std::memory_order_relaxed);
  return result;
}

void CrowPanelDisplayService::taskEntry(void* context) noexcept {
  static_cast<CrowPanelDisplayService*>(context)->taskLoop();
}

void CrowPanelDisplayService::taskLoop() noexcept {
  DisplayRenderWorkItem work_item{};
  while (true) {
    if (xQueueReceive(render_queue_, &work_item, portMAX_DELAY) != pdTRUE) {
      continue;
    }

    const bool success = renderDisplayFrameToClay(work_item.frame, geometry_, theme_, backend_);
    if (success) {
      const std::uint32_t successes =
          render_successes_.fetch_add(1U, std::memory_order_relaxed) + 1U;
      ESP_LOGI(kTag,
               "Physical refresh completed generation=%llu kind=%u reason=%u successes=%lu",
               static_cast<unsigned long long>(work_item.generation),
               static_cast<unsigned>(work_item.frame.refresh_kind),
               static_cast<unsigned>(work_item.frame.refresh_reason),
               static_cast<unsigned long>(successes));
    } else {
      const std::uint32_t failures =
          render_failures_.fetch_add(1U, std::memory_order_relaxed) + 1U;
      ESP_LOGE(kTag,
               "Physical refresh failed generation=%llu kind=%u reason=%u failures=%lu",
               static_cast<unsigned long long>(work_item.generation),
               static_cast<unsigned>(work_item.frame.refresh_kind),
               static_cast<unsigned>(work_item.frame.refresh_reason),
               static_cast<unsigned long>(failures));
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
    const std::uint32_t failures =
        submit_failures_.fetch_add(1U, std::memory_order_relaxed) + 1U;
    ESP_LOGW(kTag, "Display queue submit failed failures=%lu",
             static_cast<unsigned long>(failures));
    return;
  }
  submitted_.fetch_add(1U, std::memory_order_relaxed);
}

} // namespace growbox::app::climate_io::display
