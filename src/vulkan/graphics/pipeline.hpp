#pragma once

#include "config.hpp"
#include "utils/expected_util.hpp"
#include <vulkan/vulkan_core.h>

namespace wind::vulkan::graphics {

struct GraphicsPipelineDesc
{
};

WIND_NODISCARD auto create(const vk::raii::Device& device) WIND_NOEXCEPT -> WindResult<vk::raii::Pipeline>;

};  // namespace wind::vulkan::graphics
