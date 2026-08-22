#include "application.hpp"
#include "platform/window.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/renderer.hpp"
#include <spdlog/spdlog.h>

namespace wind {

//current life-cycle is following:
// at lower level vulkan::context owns instance, debug messenger, DeviceContext and Swapchain
// FrameContext owns N semaphores, fences and cmd buffers
// Renderer owns vulkan::context and Frame context
// Window owns its internal handler nothing else
// Application owns window and renderer
// main creates Application
// Renderer -> frame context and vulkan context
// Application -> Window and Renderer
// main -> calls Application

WIND_NODISCARD auto Application::create(platform::WindowConfiguration window_cfg, wind::vulkan::Configuration vulkan_cfg) WIND_NOEXCEPT
    -> WindResult<Application>
{
#ifdef WIND_LOG_ENABLE
  spdlog::info("initializing application...");
#endif

  auto window = platform::Window{std::move(window_cfg)};
  WIND_TRY_VOID(window.create());

  auto renderer = WIND_TRY(vulkan::Renderer::create(std::move(vulkan_cfg), window));

#ifdef WIND_LOG_ENABLE
  spdlog::info("application created successfully");
#endif

  return Application(std::move(window), std::move(renderer));
}

auto Application::run() WIND_NOEXCEPT -> WindResult<void>
{
  WIND_TRY(m_renderer.begin());

  return {};
}

auto test() -> WindResult<void>
{
  // auto vert_shader = WIND_TRY(m_resource_manager.load_shader(m_context.device.handle, "assets/shaders/vert.spv"));

  // auto frag_shader = WIND_TRY(m_resource_manager.load_shader(m_context.device.handle, "assets/shaders/frag.spv"));

  // spdlog::info("got shader handle: {}", vert_shader.index);
  // spdlog::info("got shader handle: {}", frag_shader.index);

  // auto* shader_data = WIND_TRY(m_resource_manager.get_shader(vert_shader));

  // spdlog::info("{}", (void*)shader_data);

  // // Testing
  // auto graphics_config = graphics::GraphicsConfig{.shader = {},
  //                                                 .rasterization{
  //                                                     .cull_mode    = CullMode::Back,
  //                                                     .polygon_mode = PolygonMode::Fill,
  //                                                     .front_face   = FrontFace::CounterClockwise,
  //                                                     .discard      = false,
  //                                                 },
  //                                                 .vertex_input_state{
  //                                                     .attributes{},
  //                                                     .bindings{},
  //                                                 },
  //                                                 .depth_stencil{
  //                                                     .depth_test = false,
  //                                                 },
  //                                                 .color_format = Format::RGBA8_SRGB};

  // auto handle = WIND_TRY(m_pipeline_manager.create(std::move(graphics_config), m_context.device.handle));

  // auto graphics_config_2 = graphics::GraphicsConfig{.shader = {},
  //                                                   .rasterization{
  //                                                       .cull_mode    = CullMode::Back,
  //                                                       .polygon_mode = PolygonMode::Fill,
  //                                                       .front_face   = FrontFace::CounterClockwise,
  //                                                       .discard      = false,
  //                                                   },
  //                                                   .vertex_input_state{
  //                                                       .attributes{},
  //                                                       .bindings{},
  //                                                   },
  //                                                   .depth_stencil{
  //                                                       .depth_test = false,
  //                                                   },
  //                                                   .color_format = Format::RGBA8_SRGB};

  // auto handle2 = WIND_TRY(m_pipeline_manager.create(std::move(graphics_config_2), m_context.device.handle));

  // auto p1 = WIND_TRY(m_pipeline_manager.get(handle));
  // auto p2 = WIND_TRY(m_pipeline_manager.get(handle2));

  // spdlog::info("pipeline handle: {}", handle);
  // spdlog::info("pipeline handle: {}", handle2);
  // spdlog::info("pipeline : {}", (void*)*p1->graphics_pipeline);
  // spdlog::info("pipeline : {}", (void*)*p2->graphics_pipeline);


  return {};
}

}  // namespace wind
