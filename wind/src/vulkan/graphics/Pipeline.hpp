#pragma once

#include "Config.hpp"
#include "utils/ExpectedUtil.hpp"
#include "vulkan/graphics/PipelineConfig.hpp"
#include <vulkan/vulkan_core.h>

namespace wind::vulkan::graphics {

struct GraphicsPipeline
{
  vk::raii::PipelineLayout pipeline_layout{nullptr};
  vk::raii::Pipeline       graphics_pipeline{nullptr};
};

WIND_NODISCARD auto create(const vk::raii::Device& device, GraphicsConfig cfg) WIND_NOEXCEPT -> WindResult<GraphicsPipeline>;

};  // namespace wind::vulkan::graphics
