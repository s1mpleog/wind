#pragma once

#include "Resources/Builtin.hpp"
#include "Resources/ResourceManager.hpp"
#include "Vulkan/Graphics/PipelineManager.hpp"


struct ModelComponent
{
  ModelHandle handle;
};

struct PipelineComponent
{
  PipelineHandle handle;
};

struct Transform
{
};

struct RenderObject
{
  ModelHandle           model_handle{};
  PipelineHandle pipeline_handle{};
  bool                             is_model_type{};
  BufferAssets            buffer_asset{};
  //todo: transform later
};

