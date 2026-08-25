#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace wind {

class Camera
{
public:
  Camera() = default;

  Camera(float fov, float aspect_ratio, float near_plane = 0.1F, float far_plane = 100.0F) noexcept;

  void init_perspective(float fov, float aspect_ratio, float near_plane = 0.1F, float far_plane = 100.0F) noexcept;

  void reset() noexcept;

  void update() noexcept;

  void set_position(glm::vec3 position) noexcept;
  void set_aspect_ratio(float aspect_ratio) noexcept;
  void set_fov(float fov) noexcept;
  void set_near_plane(float near_plane) noexcept;
  void set_far_plane(float far_plane) noexcept;
  void move(float forward, float right, float dt) noexcept;

  void rotate(float delta_pitch, float delta_yaw) noexcept;

  [[nodiscard]]
  const glm::vec3& position() const noexcept
  {
    return m_position;
  }

  [[nodiscard]]
  const glm::vec3& forward() const noexcept
  {
    return m_forward;
  }

  [[nodiscard]]
  const glm::vec3& right() const noexcept
  {
    return m_right;
  }

  [[nodiscard]]
  const glm::vec3& up() const noexcept
  {
    return m_up;
  }

  [[nodiscard]]
  const glm::mat4& view() const noexcept
  {
    return m_view;
  }

  [[nodiscard]]
  const glm::mat4& projection() const noexcept
  {
    return m_projection;
  }

  [[nodiscard]]
  const glm::mat4& view_projection() const noexcept
  {
    return m_view_projection;
  }

  [[nodiscard]]
  float yaw() const noexcept
  {
    return m_yaw;
  }

  [[nodiscard]]
  float pitch() const noexcept
  {
    return m_pitch;
  }

private:
  void update_view() noexcept;
  void update_projection() noexcept;

private:
  glm::vec3 m_position{0.0F, 0.0F, 10.0F};

  glm::vec3 m_forward{0.0F, 0.0F, -1.0F};
  glm::vec3 m_right{1.0F, 0.0F, 0.0F};
  glm::vec3 m_up{0.0F, 1.0F, 0.0F};

  float m_yaw{};
  float m_pitch{};

  float m_fov{60.0F};
  float m_aspect_ratio{16.0F / 9.0F};

  float m_near_plane{0.1F};
  float m_far_plane{100.0F};

  glm::mat4 m_view{1.0F};
  glm::mat4 m_projection{1.0F};
  glm::mat4 m_view_projection{1.0F};

  bool m_projection_dirty{true};
};

}  // namespace wind
