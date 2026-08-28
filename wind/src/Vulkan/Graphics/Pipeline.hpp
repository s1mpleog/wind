#pragma once

#include "Config.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Graphics/PipelineConfig.hpp"

#include <vulkan/vulkan_core.h>

struct FGraphicsPipeline
{
	vk::raii::PipelineLayout PipelineLayout{nullptr};
	vk::raii::Pipeline GraphicsPipeline{nullptr};
};

WIND_NODISCARD auto CreatePipeline(const vk::raii::Device &Device, FGraphicsConfig Cfg) WIND_NOEXCEPT
    -> WindResult<FGraphicsPipeline>;
