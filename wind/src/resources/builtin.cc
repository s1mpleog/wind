#include "builtin.hpp"
#include "glm/ext/vector_float4.hpp"
#include "resources/resource_manager.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/graphics/pipeline_config.hpp"
#include "vulkan/graphics/pipeline_manager.hpp"
#include "vulkan/graphics/shader_types.hpp"
#include <cstddef>
#include <span>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace wind::builtin {

namespace {
auto setup_default_cube(resources::ResourceManager*        resource_manager,
                        vulkan::graphics::PipelineManager* pipeline_manager,
                        const vk::raii::Device&            device) WIND_NOEXCEPT -> WindResult<BuiltAssets>
{

  using namespace wind::resources;
  using namespace wind::vulkan;

  auto cube_vert = WIND_TRY(resource_manager->create_vertex_buffer(std::as_bytes(std::span{cube_vertices})));

  auto cube_indc = WIND_TRY(resource_manager->create_index_buffer(std::as_bytes(std::span{cube_indices})));

  auto cube_vertex_handle = WIND_TRY(resource_manager->load_shader(device, "assets/shaders/triangle.vert.spv"));

  auto cube_fragment_handle = WIND_TRY(resource_manager->load_shader(device, "assets/shaders/triangle.frag.spv"));

  ShaderInfo cube_vert_shader_info{
      .stage  = ShaderStage::Vertex,
      .module = WIND_TRY(resource_manager->get_shader(cube_vertex_handle)),
  };

  ShaderInfo cube_frag_shader_info{
      .stage  = ShaderStage::Fragment,
      .module = WIND_TRY(resource_manager->get_shader(cube_fragment_handle)),
  };

  auto cube_config = graphics::GraphicsConfig{
      .shader = {cube_vert_shader_info, cube_frag_shader_info},
      .rasterization =
          {
              .cull_mode = CullMode::None,
          },
      .vertex_input_state =
          {.attributes = {{.location = 0, .binding = 0, .format = VertexFormat::Float3, .offset = offsetof(Vertex, position)},
                          {.location = 1, .binding = 0, .format = VertexFormat::Float3, .offset = offsetof(Vertex, color)}},

           .bindings = {{
               .binding    = 0,
               .stride     = sizeof(Vertex),
               .input_rate = VertexInputRate::Vertex,
           }}},
      .color_blend           = {.enabled = false},
      .push_constants        = {{
          .stage_flags = ShaderStage::Vertex,
          .offset      = 0,
          .size        = sizeof(PushConstants),
      }},
      .descriptor_set_layout = std::nullopt,
      .color_format          = Format::BGRA8_SRGB};

  auto cube_pipeline_index = WIND_TRY(pipeline_manager->create(std::move(cube_config), device));

  resource_manager->destroy_shader(cube_vertex_handle);
  resource_manager->destroy_shader(cube_fragment_handle);

  auto asset_info = BuiltAssets{.pipelines           = cube_pipeline_index,
                                .has_descriptor_sets = false,
                                .is_model            = false,

                                .buffer_assets = BufferAssets{.vertex_handle = cube_vert,
                                                              .index_handle  = cube_indc,
                                                              .index_count   = static_cast<u32>(cube_indices.size()),
                                                              .vertex_count  = static_cast<u32>(cube_vertices.size())}};
  return asset_info;
}

}  // namespace

WIND_NODISCARD auto build(resources::ResourceManager*        resource_manager,
                          vulkan::graphics::PipelineManager* pipeline_manager,
                          const vk::raii::Device& device) WIND_NOEXCEPT -> WindResult<std::vector<BuiltAssets>>
{
  using namespace wind::vulkan;

  auto vertex_shader_handle = WIND_TRY(resource_manager->load_shader(device, "assets/shaders/model.vert.spv"));

  auto fragment_shader_handle = WIND_TRY(resource_manager->load_shader(device, "assets/shaders/model.frag.spv"));

  ShaderInfo vert_info{
      .stage  = ShaderStage::Vertex,
      .module = WIND_TRY(resource_manager->get_shader(vertex_shader_handle)),
  };

  ShaderInfo frag_info{
      .stage  = ShaderStage::Fragment,
      .module = WIND_TRY(resource_manager->get_shader(fragment_shader_handle)),
  };

  auto suzanne_config =
      graphics::GraphicsConfig{.shader = {vert_info, frag_info},
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
                                               },
                                               {
                                                   .location = 3,
                                                   .binding  = 3,
                                                   .format   = VertexFormat::Float4,
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
                                             },
                                             {.binding = 3, .stride = sizeof(glm::vec4), .input_rate = VertexInputRate::Vertex}},
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
                               .color_format          = Format::BGRA8_SRGB,
                               .depth_format          = Format::D32_FLOAT};


  auto pipeline_handle = WIND_TRY(pipeline_manager->create(std::move(suzanne_config), device));

  resource_manager->destroy_shader(vertex_shader_handle);
  resource_manager->destroy_shader(fragment_shader_handle);

  auto model_handle = WIND_TRY(resource_manager->load_model("assets/models/thanos.wind"));

  std::vector<BuiltAssets> assets;

  assets.push_back(BuiltAssets{.models = model_handle, .pipelines = pipeline_handle, .has_descriptor_sets = true, .is_model = true});

  auto cube = WIND_TRY(setup_default_cube(resource_manager, pipeline_manager, device));

  assets.push_back(cube);

  return assets;
}
};  // namespace wind::builtin
