#pragma once

#include "utils/ExpectedUtil.hpp"
#include "vulkan/core/Configuration.hpp"
#include <vulkan/vulkan_core.h>

namespace wind::vulkan::instance {
WIND_NODISCARD auto create(const Configuration& cfg, const vk::raii::Context& ctx, std::vector<const char*> extensions) WIND_NOEXCEPT
    -> WindResult<vk::raii::Instance>;
}
