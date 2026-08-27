#pragma once

#include "resources/resource_manager.hpp"
#include "vulkan/graphics/pipeline_manager.hpp"
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace wind::builtin {
struct BuiltAssets
{
  resources::ModelHandle           models;
  vulkan::graphics::PipelineHandle pipelines;
};

WIND_NODISCARD auto build(resources::ResourceManager*        resource_manager,
                          vulkan::graphics::PipelineManager* pipeline_manager,
                          const vk::raii::Device& device) WIND_NOEXCEPT -> WindResult<std::vector<BuiltAssets>>;

};  // namespace wind::builtin
