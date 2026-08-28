#pragma once

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float4.hpp"

struct FPushConstants
{
	glm::mat4 Transform;
	u32 AlbedoTexture;
	u32 NormalIndex;
	u32 MetallicRoughnessIndex;
	glm::vec4 BaseColor;
};

struct FUboInstance
{
	glm::mat4 Transform;
};

struct FVertex
{
	glm::vec3 Position;
	glm::vec3 Color;
};

static_assert(sizeof(FVertex) == 24);
static_assert(offsetof(FVertex, Position) == 0);
static_assert(offsetof(FVertex, Color) == 12);

// static std::array<Vertex, 3> default_vertices{{
//     {{0.0F, 0.5F, 0.0F, 1.0F}, {1.0F, 0.0F, 0.0F, 1.0F}},
//     {{0.5F, -0.5F, 0.0F, 1.0F}, {0.0F, 1.0F, 0.0F, 1.0F}},
//     {{-0.5F, -0.5F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F, 1.0F}},
// }};

static constexpr std::array<FVertex, 8> CubeVertices{{
    // Front face
    {.Position = {-0.5F, -0.5F, 0.5F}, .Color = {1.0F, 0.0F, 0.0F}}, // 0
    {.Position = {0.5F, -0.5F, 0.5F}, .Color = {0.0F, 1.0F, 0.0F}},  // 1
    {.Position = {0.5F, 0.5F, 0.5F}, .Color = {0.0F, 0.0F, 1.0F}},   // 2
    {.Position = {-0.5F, 0.5F, 0.5F}, .Color = {1.0F, 1.0F, 0.0F}},  // 3

    // Back face
    {.Position = {-0.5F, -0.5F, -0.5F}, .Color = {1.0F, 0.0F, 1.0F}}, // 4
    {.Position = {0.5F, -0.5F, -0.5F}, .Color = {0.0F, 1.0F, 1.0F}},  // 5
    {.Position = {0.5F, 0.5F, -0.5F}, .Color = {1.0F, 1.0F, 1.0F}},   // 6
    {.Position = {-0.5F, 0.5F, -0.5F}, .Color = {0.2F, 0.2F, 0.2F}},  // 7
}};

static constexpr std::array<std::uint16_t, 36> CubeIndices{{
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

static_assert(CubeIndices.size() == 36);
