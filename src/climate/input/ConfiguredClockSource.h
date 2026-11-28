#pragma once

#include "IpoSignalSelector.h"
#include "climate/application/ClimateInputContracts.h"

namespace growbox::app::climate_io::runtime {

// Configured clock input uses the same selector as environmental channels.
// The installed RTC transport remains injected; both Rule and the scheduled
// lamp consume the same timestamp/validity without a second RTC read.
class ConfiguredClockSource final : public ClimateClockSource {
public:
  ConfiguredClockSource(const ::growbox::ipo::Graph& graph, ClimateClockSource& rtc) noexcept
      : graph_(graph), rtc_(rtc) {}

  bool sample(std::uint64_t monotonic_ms, ClimateWallClockSnapshot& output) noexcept override {
    if (!sampled_ || sampled_at_ != monotonic_ms) {
      sampled_ = true;
      sampled_at_ = monotonic_ms;
      cached_ = {};
      using namespace ::growbox::ipo;
      SignalReading reading{};
      std::size_t count = 0U;
      if (validateGraph(graph_) == GraphStatus::Ok) {
        for (std::size_t i = 0; i < graph_.device_count; ++i) {
          const auto& d = graph_.devices[i];
          if (!d.enabled || d.driver != Driver::Ds3231 || d.physical_locator != 0x68U) continue;
          ClimateWallClockSnapshot raw{};
          if (rtc_.sample(monotonic_ms, raw)) {
            // Keep Unix seconds exact in the existing double-valued selector.
            constexpr std::uint64_t max_exact = 9007199254740991ULL;
            reading = {d.id, Channel::UnixTime, static_cast<double>(raw.unix_time_s),
                       0U, raw.valid && raw.unix_time_s <= max_exact};
            count = 1U;
          }
          break;
        }
      }
      selected_ = selectFreshSignal(graph_, SignalRole::ClockTime, &reading, count, 0U);
      if (selected_.usable()) {
        cached_ = {true, static_cast<std::uint64_t>(selected_.value)};
      }
    }
    output = cached_;
    return output.valid;
  }

  const ::growbox::ipo::SignalSelection& lastSelection() const noexcept { return selected_; }

private:
  const ::growbox::ipo::Graph& graph_;
  ClimateClockSource& rtc_;
  ClimateWallClockSnapshot cached_{};
  ::growbox::ipo::SignalSelection selected_{};
  std::uint64_t sampled_at_{0U};
  bool sampled_{false};
};

} // namespace growbox::app::climate_io::runtime
