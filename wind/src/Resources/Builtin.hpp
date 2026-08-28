#pragma once

#include "Resources/ResourceManager.hpp"
#include "Vulkan/Graphics/PipelineManager.hpp"
#include <vector>
#include <vulkan/vulkan_raii.hpp>


struct ModelAssets
{
  // descriptor set and push constant exists for model
  ModelHandle model{};
};

struct BufferAssets
{
  BufferHandle vertex_handle{};
  BufferHandle index_handle{};

  u32 index_count{};
  u32 vertex_count{};
};

struct BuiltAssets
{
  ModelHandle    models{};
  PipelineHandle pipelines{};
  bool           has_descriptor_sets{false};
  bool           is_model{true};
  BufferAssets   buffer_assets{};
};

WIND_NODISCARD auto build(ResourceManager* resource_manager, PipelineManager* pipeline_manager, const vk::raii::Device& device) WIND_NOEXCEPT
    -> WindResult<std::vector<BuiltAssets>>;
