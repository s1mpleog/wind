#pragma once

#include "glm/ext/matrix_float4x4.hpp"

struct PushConstants
{
  glm::mat4 transform;
  u32       albedo_texture;
};
