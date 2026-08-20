#pragma once

#include "utils/expected_util.hpp"
#include "vulkan/core/configuration.hpp"
#include <vulkan/vulkan_core.h>

namespace wind::vulkan::instance {
WIND_NODISCARD auto create(const Configuration& cfg, const vk::raii::Context& ctx, std::vector<const char*> extensions) WIND_NOEXCEPT
    -> WindResult<vk::raii::Instance>;
}
