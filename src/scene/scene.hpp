#pragma once


// scene will own all the render_object and camera
// but someone have to push all the handles into scene

#include "camera.hpp"
#include "render_object.hpp"
#include "resources/resource_manager.hpp"
#include "vulkan/graphics/pipeline_manager.hpp"
#include <vector>

namespace wind::scene {
class Scene
{
public:
  auto add_render_objects(resources::ModelHandle model_handle, vulkan::graphics::PipelineHandle pipeline_handle, Transform = {}) WIND_NOEXCEPT
      -> void;

  auto get() WIND_NOEXCEPT -> std::vector<RenderObject>;

private:
  std::vector<RenderObject> m_objects;
  Camera                    m_camera;
};

};  // namespace wind::scene
