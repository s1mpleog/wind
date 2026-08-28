#pragma once

#include "Resources/ResourceManager.hpp"
#include "Vulkan/Graphics/PipelineManager.hpp"
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace wind::builtin {

struct ModelAssets
{
  // descriptor set and push constant exists for model
  resources::ModelHandle model{};
};

struct BufferAssets
{
  resources::BufferHandle vertex_handle{};
  resources::BufferHandle index_handle{};

  u32 index_count{};
  u32 vertex_count{};
};

struct BuiltAssets
{
  resources::ModelHandle           models{};
  vulkan::graphics::PipelineHandle pipelines{};
  bool                             has_descriptor_sets{false};
  bool                             is_model{true};
  BufferAssets                     buffer_assets{};
};

WIND_NODISCARD auto build(resources::ResourceManager*        resource_manager,
                          vulkan::graphics::PipelineManager* pipeline_manager,
                          const vk::raii::Device& device) WIND_NOEXCEPT -> WindResult<std::vector<BuiltAssets>>;

};  // namespace wind::builtin
