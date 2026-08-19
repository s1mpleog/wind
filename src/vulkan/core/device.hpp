#pragma once

#include "utils/expected_util.hpp"
#include "vulkan/core/configuration.hpp"
#include <vulkan/vulkan_core.h>

namespace wind::vulkan::device {
[[nodiscard]] auto init(const Configuration& cfg, const vk::raii::Instance& instance, const vk::raii::SurfaceKHR& surface) noexcept
    -> WindResult<vk::raii::Device>;
};
