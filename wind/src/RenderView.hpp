#pragma once

#include "glm/ext/matrix_float4x4.hpp"

namespace wind {
struct RenderView
{
  glm::mat4 view;
  glm::mat4 projection;
};

}  // namespace wind
