#pragma once

#include "ClimatePolicyTypes.h"

#include <cstdint>

namespace growbox::climate {

namespace policy {
[[nodiscard]] ClimatePolicyRequest ruleRequest(const ClimateControllerInput& input) noexcept;
void evaluate(const ClimatePolicyRequest& raw, const ClimateControllerInput& input,
              ClimatePolicyEvaluation& evaluation) noexcept;
[[nodiscard]] bool finiteRequest(const ClimatePolicyRequest& request) noexcept;
[[nodiscard]] ClimatePolicyRequest applyDeadzone(const ClimatePolicyRequest& raw,
                                                 float threshold) noexcept;
[[nodiscard]] ClimatePolicyRequest clipped(const ClimatePolicyRequest& request) noexcept;
} // namespace policy

} // namespace growbox::climate
