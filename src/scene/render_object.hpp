#pragma once

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
  //todo: transform later
};

}  // namespace wind::scene
