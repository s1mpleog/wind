#include "camera.hpp"
#include "glm/ext/matrix_clip_space.hpp"

#include <algorithm>
#include <cmath>

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/scalar_constants.hpp>

namespace wind {

Camera::Camera(float fov, float aspect_ratio, float near_plane, float far_plane) noexcept
{
  init_perspective(fov, aspect_ratio, near_plane, far_plane);
}

void Camera::move(float forward_amount, float right_amount, float dt) noexcept
{
  glm::vec3 movement{};

  // Horizontal forward direction.
  glm::vec3 horizontal_forward{m_forward.x, 0.0F, m_forward.z};

  if(glm::length(horizontal_forward) > 0.0F)
    horizontal_forward = glm::normalize(horizontal_forward);

  movement += horizontal_forward * forward_amount;
  movement += m_right * right_amount;

  m_position += movement * dt;
}

void Camera::init_perspective(float fov, float aspect_ratio, float near_plane, float far_plane) noexcept
{
  m_fov          = fov;
  m_aspect_ratio = aspect_ratio;
  m_near_plane   = near_plane;
  m_far_plane    = far_plane;

  reset();
}

void Camera::reset() noexcept
{
  m_position = {0.0F, 0.0F, 3.0F};

  m_yaw   = 0.0F;
  m_pitch = 0.0F;

  m_forward = {0.0F, 0.0F, -1.0F};
  m_right   = {1.0F, 0.0F, 0.0F};
  m_up      = {0.0F, 1.0F, 0.0F};

  m_view            = glm::mat4{1.0F};
  m_projection      = glm::mat4{1.0F};
  m_view_projection = glm::mat4{1.0F};

  m_projection_dirty = true;
}

void Camera::update() noexcept
{
  update_view();

  if(m_projection_dirty)
  {
    update_projection();
    m_projection_dirty = false;
  }

  m_view_projection = m_projection * m_view;
}

void Camera::update_view() noexcept
{
  // Forward direction from yaw/pitch.
  //
  // yaw   = 0
  // pitch = 0
  //
  // gives:
  //
  // forward = (0, 0, -1)
  //
  const float cos_pitch = std::cos(m_pitch);
  const float sin_pitch = std::sin(m_pitch);

  const float cos_yaw = std::cos(m_yaw);
  const float sin_yaw = std::sin(m_yaw);

  m_forward = glm::normalize(glm::vec3{
      cos_pitch * sin_yaw,
      sin_pitch,
      -cos_pitch * cos_yaw,
  });

  // Camera basis.
  //
  // World up is +Y.
  //
  m_right = glm::normalize(glm::cross(m_forward, glm::vec3{0.0F, 1.0F, 0.0F}));

  m_up = glm::normalize(glm::cross(m_right, m_forward));

  m_view = glm::lookAt(m_position, m_position + m_forward, m_up);
}

void Camera::update_projection() noexcept
{
  m_projection = glm::perspective(glm::radians(m_fov), m_aspect_ratio, m_near_plane, m_far_plane);
}

void Camera::set_position(glm::vec3 position) noexcept
{
  m_position = position;
}

void Camera::set_aspect_ratio(float aspect_ratio) noexcept
{
  if(aspect_ratio <= 0.0F)
    return;

  m_aspect_ratio     = aspect_ratio;
  m_projection_dirty = true;
}

void Camera::set_fov(float fov) noexcept
{
  m_fov              = std::clamp(fov, 1.0F, 179.0F);
  m_projection_dirty = true;
}

void Camera::set_near_plane(float near_plane) noexcept
{
  m_near_plane       = near_plane;
  m_projection_dirty = true;
}

void Camera::set_far_plane(float far_plane) noexcept
{
  m_far_plane        = far_plane;
  m_projection_dirty = true;
}

void Camera::rotate(float delta_pitch, float delta_yaw) noexcept
{
  m_pitch += delta_pitch;
  m_yaw += delta_yaw;

  // Prevent the camera from reaching +/- 90 degrees.
  constexpr float limit = glm::half_pi<float>() - 0.001F;

  m_pitch = std::clamp(m_pitch, -limit, limit);
}

}  // namespace wind
