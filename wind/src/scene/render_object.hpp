#pragma once

#include "resources/builtin.hpp"
#include "resources/resource_manager.hpp"
#include "vulkan/graphics/pipeline_manager.hpp"

namespace wind::scene {

struct ModelComponent
{
  resources::ModelHandle handle;
};

struct PipelineComponent
{
  vulkan::graphics::PipelineHandle handle;
};

struct Transform
{
};

struct RenderObject
{
  resources::ModelHandle           model_handle{};
  vulkan::graphics::PipelineHandle pipeline_handle{};
  bool                             is_model_type{};
  builtin::BufferAssets            buffer_asset{};
  //todo: transform later
};

}  // namespace wind::scene
