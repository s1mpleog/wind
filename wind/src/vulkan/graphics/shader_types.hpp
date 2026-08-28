#pragma once

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float4.hpp"

struct PushConstants
{
  glm::mat4 transform;
  u32       albedo_texture;
  u32       normal_index;
  u32       metallic_roughness_index;
  glm::vec4 base_color;
};

struct UboInstance
{
  glm::mat4 transform;
};

struct Vertex
{
  glm::vec3 position;
  glm::vec3 color;
};

static_assert(sizeof(Vertex) == 24);
static_assert(offsetof(Vertex, position) == 0);
static_assert(offsetof(Vertex, color) == 12);

// static std::array<Vertex, 3> default_vertices{{
//     {{0.0F, 0.5F, 0.0F, 1.0F}, {1.0F, 0.0F, 0.0F, 1.0F}},
//     {{0.5F, -0.5F, 0.0F, 1.0F}, {0.0F, 1.0F, 0.0F, 1.0F}},
//     {{-0.5F, -0.5F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F, 1.0F}},
// }};

static constexpr std::array<Vertex, 8> cube_vertices{{
    // Front face
    {{-0.5F, -0.5F, 0.5F}, {1.0F, 0.0F, 0.0F}},  // 0
    {{0.5F, -0.5F, 0.5F}, {0.0F, 1.0F, 0.0F}},   // 1
    {{0.5F, 0.5F, 0.5F}, {0.0F, 0.0F, 1.0F}},    // 2
    {{-0.5F, 0.5F, 0.5F}, {1.0F, 1.0F, 0.0F}},   // 3

    // Back face
    {{-0.5F, -0.5F, -0.5F}, {1.0F, 0.0F, 1.0F}},  // 4
    {{0.5F, -0.5F, -0.5F}, {0.0F, 1.0F, 1.0F}},   // 5
    {{0.5F, 0.5F, -0.5F}, {1.0F, 1.0F, 1.0F}},    // 6
    {{-0.5F, 0.5F, -0.5F}, {0.2F, 0.2F, 0.2F}},   // 7
}};

static constexpr std::array<std::uint16_t, 36> cube_indices{{
    // Front
    0,
    1,
    2,
    2,
    3,
    0,

    // Right
    1,
    5,
    6,
    6,
    2,
    1,

    // Back
    5,
    4,
    7,
    7,
    6,
    5,

    // Left
    4,
    0,
    3,
    3,
    7,
    4,

    // Top
    3,
    2,
    6,
    6,
    7,
    3,

    // Bottom
    4,
    5,
    1,
    1,
    0,
    4,
}};

static_assert(cube_indices.size() == 36);
