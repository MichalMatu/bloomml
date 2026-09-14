// CRITICAL: Define LOG_LOCAL_LEVEL before any ESP-IDF headers
#if defined(ESP_PLATFORM)
#ifndef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#endif
#endif

#include <array>
#include "epd2_9/core/HardwarePins.h"
#include "epd2_9/epd2_9.h"
#include "epd2_9/drivers/input/InputDriver.h"
#include "epd2_9/drivers/input/LilyGoTouchDriver.h"
#include "epd2_9/drivers/input/TouchGestureMapper.h"

#if defined(ESP_PLATFORM)
#include "system/logging/LogMacros.h"
#else
#include <cstdio>
#endif

namespace epd2_9 {

#if !defined(ESP_PLATFORM)
static constexpr const char* kInputLogTag = "InputDriver";
#endif

// Per-key debounce
static constexpr uint32_t INPUT_DEBOUNCE_MS = 120;
static constexpr uint32_t CROWN_LONG_PRESS_MS = 700;

struct KeyState {
  uint8_t pin;
  bool active;
  bool last_level;
  uint32_t last_change_ms;
};

struct CrownState {
  bool pressed;
  bool long_emitted;
  uint32_t press_ms;
};

constexpr size_t INDEX_HOME = 0;
constexpr size_t INDEX_BACK = 1;
constexpr size_t INDEX_PREV = 2;
constexpr size_t INDEX_NEXT = 3;
constexpr size_t INDEX_CROWN = 4;

static std::array<KeyState, 5> g_keys{};
static CrownState g_crown{false, false, 0};
static LilyGoTouchDriver g_touch;
static TouchGestureMapper g_touchMapper;
static bool g_touchActive = false;
static bool g_inited = false;

const char* inputEventLabel(InputEventType type)
{
  switch (type) {
    case InputEventType::None:
      return "None";
    case InputEventType::CrownUp:
      return "CrownUp";
    case InputEventType::CrownDown:
      return "CrownDown";
    case InputEventType::CrownPress:
      return "CrownPress";
    case InputEventType::CrownLongPress:
      return "CrownLongPress";
    case InputEventType::Home:
      return "Home";
    case InputEventType::Back:
      return "Back";
  }
  return "Unknown";
}

static void logInputEvent(InputEventType type, uint32_t ms)
{
  (void)type;
  (void)ms;
#if defined(ESP_PLATFORM)
  // APP_LOGI(LogComponent::Ui,
  //          "Input event: %s (timestamp=%u ms)",
  //          inputEventLabel(type),
  //          static_cast<unsigned>(ms));
#else
  // std::printf("[%s] event=%s (%u ms)\n",
  //             kInputLogTag,
  //             inputEventLabel(type),
  //             static_cast<unsigned>(ms));
#endif
}

static KeyState make_key(uint8_t pin)
{
  return KeyState{pin, is_available_gpio(pin), HIGH, 0};
}

static void prime_key_state(KeyState& key)
{
  if (!key.active) {
    return;
  }
  pinMode(key.pin, INPUT_PULLUP);
  key.last_level = digitalRead(key.pin);
  key.last_change_ms = millis();
}

void input_init(const Config& cfg)
{
  g_keys = { make_key((uint8_t)cfg.key_home), make_key((uint8_t)cfg.key_back),
             make_key((uint8_t)cfg.key_prev), make_key((uint8_t)cfg.key_next),
             make_key((uint8_t)cfg.key_ok) };

  for (auto& key : g_keys) {
    prime_key_state(key);
  }

  g_crown.pressed = (g_keys[INDEX_CROWN].last_level == LOW);
  g_crown.long_emitted = false;
  g_crown.press_ms = millis();
  g_touchMapper.reset();
  g_touchActive = g_touch.begin(cfg);
  g_inited = true;

#if defined(ESP_PLATFORM)
  size_t activeKeys = 0;
  for (const auto& key : g_keys) {
    if (key.active) {
      ++activeKeys;
    }
  }
  APP_LOGI(LogComponent::Ui,
           "InputDriver initialized (%u keys configured, touch=%s, debounce=%ums)",
           static_cast<unsigned>(activeKeys),
           g_touchActive ? "enabled" : "disabled",
           static_cast<unsigned>(INPUT_DEBOUNCE_MS));
#else
  size_t activeKeys = 0;
  for (const auto& key : g_keys) {
    if (key.active) {
      ++activeKeys;
    }
  }
  std::printf("[%s] InputDriver initialized (%zu keys configured, touch=%s)\n",
              kInputLogTag,
              activeKeys,
              g_touchActive ? "enabled" : "disabled");
#endif
}

static bool debounce_edge(KeyState& key, bool& fell)
{
  if (!key.active) {
    return false;
  }
  bool level = digitalRead(key.pin);
  uint32_t now = millis();
  if (level != key.last_level) {
    if (now - key.last_change_ms >= INPUT_DEBOUNCE_MS) {
      fell = (key.last_level == HIGH && level == LOW);
      key.last_level = level;
      key.last_change_ms = now;
      return true;
    }
  }
  return false;
}

static bool try_emit_button_event(size_t index, InputEventType type, InputEvent& out, uint32_t now)
{
  bool fell = false;
  if (debounce_edge(g_keys[index], fell) && fell) {
    out = {type, now};
    logInputEvent(type, now);
    return true;
  }
  return false;
}

static bool handle_crown(InputEvent& out, uint32_t now)
{
  bool fell = false;
  bool edge = debounce_edge(g_keys[INDEX_CROWN], fell);
  if (edge) {
    if (fell) {
      g_crown.pressed = true;
      g_crown.press_ms = now;
      g_crown.long_emitted = false;
    } else {
      bool was_long = g_crown.long_emitted;
      g_crown.pressed = false;
      g_crown.long_emitted = false;
      g_crown.press_ms = now;
      if (!was_long) {
        out = {InputEventType::CrownPress, now};
        logInputEvent(out.type, now);
        return true;
      }
    }
  }

  if (g_crown.pressed && !g_crown.long_emitted) {
    if (now - g_crown.press_ms >= CROWN_LONG_PRESS_MS) {
      g_crown.long_emitted = true;
      out = {InputEventType::CrownLongPress, now};
      logInputEvent(out.type, now);
      return true;
    }
  }

  return false;
}

static bool handle_touch(InputEvent& out, uint32_t now)
{
  if (!g_touchActive) {
    return false;
  }

  TouchSample sample{};
  if (!g_touch.poll(sample, now)) {
    return false;
  }
  if (!g_touchMapper.update(sample, out)) {
    return false;
  }

  logInputEvent(out.type, now);
  return true;
}

bool input_poll(InputEvent& out)
{
  if (!g_inited) {
    out = {InputEventType::None, millis()};
    return false;
  }

  uint32_t now = millis();

  if (try_emit_button_event(INDEX_HOME, InputEventType::Home, out, now)) return true;
  if (try_emit_button_event(INDEX_BACK, InputEventType::Back, out, now)) return true;

  if (handle_crown(out, now)) {
    return true;
  }

  if (try_emit_button_event(INDEX_NEXT, InputEventType::CrownDown, out, now)) return true;
  if (try_emit_button_event(INDEX_PREV, InputEventType::CrownUp, out, now)) return true;
  if (handle_touch(out, now)) return true;

  out = {InputEventType::None, now};
  return false;
}

} // namespace epd2_9
