#include "climate/display/input/CrowPanelButtonInput.h"

#include <driver/gpio.h>
#include <esp_err.h>

#include <algorithm>

namespace growbox::app::climate_io::display {
namespace {

std::uint64_t monotonicMs() noexcept {
  const std::int64_t now_us = esp_timer_get_time();
  return now_us > 0 ? static_cast<std::uint64_t>(now_us) / 1000U : 0U;
}

} // namespace

CrowPanelButtonInput::CrowPanelButtonInput(CrowPanelButtonInputConfig config) noexcept
    : config_(config),
      pins_{config.pins.home, config.pins.back, config.pins.previous, config.pins.next,
            config.pins.ok},
      buttons_{DisplayButton::Home, DisplayButton::Back, DisplayButton::Previous,
               DisplayButton::Next, DisplayButton::Ok},
      states_{DisplayButtonStateMachine(config.timing), DisplayButtonStateMachine(config.timing),
              DisplayButtonStateMachine(config.timing), DisplayButtonStateMachine(config.timing),
              DisplayButtonStateMachine(config.timing)} {}

CrowPanelButtonInput::~CrowPanelButtonInput() noexcept {
  stop();
}

bool CrowPanelButtonInput::configValid() const noexcept {
  if (config_.sample_period_ms == 0U || config_.timing.debounce_ms == 0U ||
      config_.timing.long_press_ms < config_.timing.debounce_ms) {
    return false;
  }

  for (std::size_t left = 0U; left < pins_.size(); ++left) {
    if (!GPIO_IS_VALID_GPIO(pins_[left])) {
      return false;
    }
    for (std::size_t right = left + 1U; right < pins_.size(); ++right) {
      if (pins_[left] == pins_[right]) {
        return false;
      }
    }
  }
  return true;
}

bool CrowPanelButtonInput::begin() noexcept {
  if (started_) {
    return true;
  }
  if (!configValid()) {
    return false;
  }

  gpio_config_t gpio_cfg{};
  for (const int pin : pins_) {
    gpio_cfg.pin_bit_mask |= (1ULL << static_cast<unsigned>(pin));
  }
  gpio_cfg.mode = GPIO_MODE_INPUT;
  gpio_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_cfg.intr_type = GPIO_INTR_DISABLE;
  if (gpio_config(&gpio_cfg) != ESP_OK) {
    return false;
  }

  if (queue_ == nullptr) {
    queue_ = xQueueCreateStatic(kQueueDepth, sizeof(DisplayButtonEvent), queue_storage_.data(),
                                &queue_control_);
  } else {
    (void)xQueueReset(queue_);
  }
  if (queue_ == nullptr) {
    return false;
  }

  const std::uint64_t now_ms = monotonicMs();
  for (std::size_t index = 0U; index < pins_.size(); ++index) {
    states_[index].reset(gpio_get_level(static_cast<gpio_num_t>(pins_[index])) == 0, now_ms);
  }

  esp_timer_create_args_t timer_args{};
  timer_args.callback = &CrowPanelButtonInput::timerEntry;
  timer_args.arg = this;
  timer_args.dispatch_method = ESP_TIMER_TASK;
  timer_args.name = "eink_keys";
  if (esp_timer_create(&timer_args, &timer_) != ESP_OK || timer_ == nullptr) {
    timer_ = nullptr;
    return false;
  }
  if (esp_timer_start_periodic(timer_, static_cast<std::uint64_t>(config_.sample_period_ms) * 1000U) !=
      ESP_OK) {
    (void)esp_timer_delete(timer_);
    timer_ = nullptr;
    return false;
  }

  started_ = true;
  return true;
}

bool CrowPanelButtonInput::poll(DisplayButtonEvent& event) noexcept {
  if (queue_ == nullptr) {
    event = {};
    return false;
  }
  return xQueueReceive(queue_, &event, 0U) == pdTRUE;
}

CrowPanelButtonInputStatus CrowPanelButtonInput::status() const noexcept {
  return {
      started_,
      events_emitted_.load(std::memory_order_relaxed),
      long_presses_.load(std::memory_order_relaxed),
      queue_drops_.load(std::memory_order_relaxed),
  };
}

void CrowPanelButtonInput::timerEntry(void* context) noexcept {
  if (context != nullptr) {
    static_cast<CrowPanelButtonInput*>(context)->sample();
  }
}

void CrowPanelButtonInput::sample() noexcept {
  if (!started_ || queue_ == nullptr) {
    return;
  }

  const std::uint64_t now_ms = monotonicMs();
  for (std::size_t index = 0U; index < pins_.size(); ++index) {
    DisplayButtonEvent event{};
    const bool pressed = gpio_get_level(static_cast<gpio_num_t>(pins_[index])) == 0;
    if (!states_[index].update(buttons_[index], pressed, now_ms, event)) {
      continue;
    }

    if (xQueueSend(queue_, &event, 0U) != pdTRUE) {
      queue_drops_.fetch_add(1U, std::memory_order_relaxed);
      continue;
    }
    events_emitted_.fetch_add(1U, std::memory_order_relaxed);
    if (event.gesture == DisplayButtonGesture::LongPress) {
      long_presses_.fetch_add(1U, std::memory_order_relaxed);
    }
  }
}

void CrowPanelButtonInput::stop() noexcept {
  started_ = false;
  if (timer_ != nullptr) {
    (void)esp_timer_stop(timer_);
    (void)esp_timer_delete(timer_);
    timer_ = nullptr;
  }
}

} // namespace growbox::app::climate_io::display
