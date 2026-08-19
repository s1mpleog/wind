#pragma once

#include "utils/expected_util.hpp"
#include "vulkan/core/configuration.hpp"
#include <vulkan/vulkan_core.h>

namespace wind::vulkan {
[[nodiscard]] auto create_instance(const Configuration& cfg, const vk::raii::Context& ctx, std::vector<const char*> extensions) noexcept
    -> WindResult<vk::raii::Instance>;
}
