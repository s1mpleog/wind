#pragma once

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float4.hpp"

struct PushConstants
{
  glm::mat4 transform;
  u32       albedo_texture;
  float     _pad[3];
  glm::vec4 base_color;
};
