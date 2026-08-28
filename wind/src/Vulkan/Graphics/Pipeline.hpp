#pragma once

#include "Config.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Graphics/PipelineConfig.hpp"
#include <vulkan/vulkan_core.h>


struct GraphicsPipeline
{
  vk::raii::PipelineLayout pipeline_layout{nullptr};
  vk::raii::Pipeline       graphics_pipeline{nullptr};
};

WIND_NODISCARD auto create_pipeline(const vk::raii::Device& device, GraphicsConfig cfg) WIND_NOEXCEPT
    -> WindResult<GraphicsPipeline>;
