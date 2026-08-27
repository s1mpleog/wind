#include "builtin.hpp"
#include "resources/resource_manager.hpp"
#include "spdlog/spdlog.h"
#include "utils/expected_util.hpp"
#include "vulkan/graphics/pipeline.hpp"
#include "vulkan/graphics/pipeline_config.hpp"
#include "vulkan/graphics/shader_types.hpp"
#include <string_view>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace wind::builtin {
WIND_NODISCARD auto build(resources::ResourceManager*        resource_manager,
                          vulkan::graphics::PipelineManager* pipeline_manager,
                          const vk::raii::Device& device) WIND_NOEXCEPT -> WindResult<std::vector<BuiltAssets>>
{
  using namespace wind::vulkan;

  auto vertex_shader_handle = WIND_TRY(resource_manager->load_shader(device, "assets/shaders/suzanne.vert.spv"));

  auto fragment_shader_handle = WIND_TRY(resource_manager->load_shader(device, "assets/shaders/suzanne.frag.spv"));

  ShaderInfo vert_info{
      .stage  = ShaderStage::Vertex,
      .module = WIND_TRY(resource_manager->get_shader(vertex_shader_handle)),
  };

  ShaderInfo frag_info{
      .stage  = ShaderStage::Fragment,
      .module = WIND_TRY(resource_manager->get_shader(fragment_shader_handle)),
  };

  auto suzanne_config = graphics::GraphicsConfig{.shader = {vert_info, frag_info},
                                                 .rasterization{
                                                     .cull_mode    = CullMode::Back,
                                                     .polygon_mode = PolygonMode::Fill,
                                                     .front_face   = FrontFace::CounterClockwise,
                                                     .depth_clamp  = false,
                                                     .discard      = false,
                                                 },
                                                 .vertex_input_state{
                                                     .attributes{{
                                                                     .location = 0,
                                                                     .binding  = 0,
                                                                     .format   = VertexFormat::Float3,
                                                                     .offset   = 0,
                                                                 },
                                                                 {
                                                                     .location = 1,
                                                                     .binding  = 1,
                                                                     .format   = VertexFormat::Float3,
                                                                     .offset   = 0,
                                                                 },
                                                                 {
                                                                     .location = 2,
                                                                     .binding  = 2,
                                                                     .format   = VertexFormat::Float2,
                                                                     .offset   = 0,
                                                                 }},
                                                     .bindings{{
                                                                   .binding    = 0,
                                                                   .stride     = sizeof(glm::vec3),
                                                                   .input_rate = VertexInputRate::Vertex,
                                                               },
                                                               {
                                                                   .binding    = 1,
                                                                   .stride     = sizeof(glm::vec3),
                                                                   .input_rate = VertexInputRate::Vertex,
                                                               },
                                                               {
                                                                   .binding    = 2,
                                                                   .stride     = sizeof(glm::vec2),
                                                                   .input_rate = VertexInputRate::Vertex,
                                                               }},
                                                 },
                                                 .input_assembly{.topology = PrimitiveTopology::TriangleList},
                                                 .depth_stencil{
                                                     .depth_test    = true,
                                                     .depth_write   = true,
                                                     .depth_compare = CompareOp::Less,
                                                 },
                                                 .color_blend = {.enabled = false},
                                                 .push_constants{{
                                                     .stage_flags = ShaderStage::Vertex | ShaderStage::Fragment,
                                                     .offset      = 0,
                                                     .size        = sizeof(PushConstants),
                                                 }},
                                                 .descriptor_set_layout = *resource_manager->get_bindless_descriptor_layout(),
                                                 .color_format = Format::BGRA8_SRGB,
                                                 .depth_format = Format::D32_FLOAT};


  auto pipeline_handle = WIND_TRY(pipeline_manager->create(std::move(suzanne_config), device));

  resource_manager->destroy_shader(vertex_shader_handle);
  resource_manager->destroy_shader(fragment_shader_handle);

  auto model_handle = WIND_TRY(resource_manager->load_model("assets/models/head.wind"));

  std::vector<BuiltAssets> assets;

  assets.push_back(BuiltAssets{.models = model_handle, .pipelines = pipeline_handle});

  return assets;
}

};  // namespace wind::builtin
