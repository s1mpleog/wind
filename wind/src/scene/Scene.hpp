#pragma once


// scene will own all the render_object and camera
// but someone have to push all the handles into scene

#include "./Camera.hpp"
#include "scene/RenderObject.hpp"
#include <vector>

namespace wind::scene {
class Scene
{
public:
  auto add_render_objects(builtin::BuiltAssets assets, bool is_model = true, Transform transform = {}) WIND_NOEXCEPT -> void;

  auto get() WIND_NOEXCEPT -> std::vector<RenderObject>;

  Scene()
  {
    camera = Camera{.position = {0.0F, 1.75F, 7.5F},
                    // .position = {0.0f, 0.0f, 3.0f},
                    .yaw   = glm::radians(180.0F),
                    .pitch = glm::radians(-9.0F),
                    // .pitch = glm::radians(0.0f),
                    .fov    = 60.0F,
                    .aspect = static_cast<float>(1200.0F) / static_cast<float>(700.0F),
                    .near   = 0.1F,
                    .far    = 1000.0f};
  }


  // make camera public for now it does not have any dependency to hide
  Camera camera;

private:
  std::vector<RenderObject> m_objects;
};

};  // namespace wind::scene
