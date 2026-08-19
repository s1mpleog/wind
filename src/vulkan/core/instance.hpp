#pragma once

#include "utils/expected_util.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace wind::vulkan {
[[nodiscard]] auto create_instance(const vk::raii::Context& ctx, std::vector<const char*> extensions) noexcept
    -> WindResult<vk::raii::Instance>;
}
