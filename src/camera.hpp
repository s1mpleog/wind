#pragma once

#include "SDL3/SDL_scancode.h"
#include "core/service_locator.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/trigonometric.hpp"
#include "input/input_manager.hpp"
#include "render_view.hpp"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace wind {
struct Camera
{
  glm::vec3 position{0.0F, 1.75F, 7.5F};
  float     yaw{glm::radians(180.0F)};
  float     pitch{glm::radians(-9.0F)};

  float fov                      = 60.0F;
  float aspect                   = 16.0F / 9.0F;
  float near                     = 0.1F;
  float far                      = 1000.0F;
  bool  ignore_next_mouse_motion = true;

  WIND_NODISCARD auto forward() const WIND_NOEXCEPT -> glm::vec3
  {
    return glm::normalize(glm::vec3(glm::cos(pitch) * glm::sin(yaw), glm::sin(pitch), glm::cos(pitch) * glm::cos(yaw)));
  }

  // auto right() const -> glm::vec3 { return glm::normalize(glm::cross(glm::vec3(0.0F, 1.0F, 0.0F), forward())); }

  auto right() const -> glm::vec3 { return glm::normalize(glm::cross(forward(), glm::vec3(0.0F, 1.0F, 0.0F))); }

  auto view() const -> glm::mat4 { return glm::lookAt(position, position + forward(), glm::vec3(0.0F, 1.0F, 0.0F)); }

  auto skybox_view() const -> glm::mat4 { return glm::mat4(glm::mat3(view())); }

  WIND_NODISCARD auto projection() const WIND_NOEXCEPT -> glm::mat4
  {
    auto proj = glm::perspective(glm::radians(fov), aspect, near, far);
    proj[1][1] *= -1.0F;  // Vulkan Y-Flip
    return proj;
  }

  auto view_proj() const -> glm::mat4 { return projection() * view(); }

  auto update_aspect(u32 width, u32 height) WIND_NOEXCEPT -> void
  {
    aspect = static_cast<float>(width) / static_cast<float>(height);
  }

  WIND_NODISCARD auto render_view() const WIND_NOEXCEPT -> RenderView
  {
    return RenderView{
        .view       = view(),
        .projection = projection(),
    };
  }

  auto process_keyboard(float delta) -> void
  {
    const float speed = 10.0F * delta;

    const glm::vec3 f = forward();
    const glm::vec3 r = right();
    const glm::vec3 u = glm::normalize(glm::cross(r, f));  // proper up

    auto& input = core::ServiceLocator::get<input::InputManger>();

    if(input.is_down(SDL_SCANCODE_W))
      position += f * speed;  // Forward

    if(input.is_down(SDL_SCANCODE_S))
      position -= f * speed;  // Backward

    if(input.is_down(SDL_SCANCODE_A))
      position -= r * speed;  // Left

    if(input.is_down(SDL_SCANCODE_D))
      position += r * speed;  // Right
  }

  auto process_mouse() -> void
  {
    if(ignore_next_mouse_motion)
    {
      ignore_next_mouse_motion = false;
      return;
    }

    auto& input = core::ServiceLocator::get<input::InputManger>();

    const auto mouse = input.get_mouse_position();

    constexpr float sensitivity = 0.0025F;

    yaw += mouse.x_rel * sensitivity;
    pitch -= mouse.y_rel * sensitivity;

    pitch = glm::clamp(pitch, glm::radians(-89.0F), glm::radians(89.0F));
  }

  auto reset_mouse_ignore() -> void { ignore_next_mouse_motion = true; }
};
}  // namespace wind
