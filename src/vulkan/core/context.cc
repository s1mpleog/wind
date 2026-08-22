#include "context.hpp"
#include "resources/resource_manager.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/core/device.hpp"
#include "vulkan/core/instance.hpp"
#include "vulkan/core/validation_layer.hpp"
#include "swapchain.hpp"
#include "vulkan/graphics/pipeline.hpp"
#include "vulkan/graphics/pipeline_config.hpp"

namespace wind::vulkan {
auto create_context(const platform::Window& window, Configuration cfg) WIND_NOEXCEPT -> WindResult<Context>
{
  Context ctx{};

  auto platform_extensions = WIND_TRY(window.extensions());
  ctx.instance             = WIND_TRY(instance::create(cfg, ctx.raii_ctx, std::move(platform_extensions)));

#ifdef WIND_VULKAN_VALIDATION
#ifdef WIND_LOG_ENABLE
  spdlog::info("WIND_VULKAN_VALIDATION enabled, setting up debug messenger");
#endif
  ctx.messenger = WIND_TRY(create_debug_utils(cfg, ctx.instance));
#endif

  auto* surface_raw = WIND_TRY(window.create_surface(ctx.instance));
  ctx.surface       = vk::raii::SurfaceKHR(ctx.instance, surface_raw);

  ctx.device_ctx = WIND_TRY(device::create(cfg, ctx.instance, ctx.surface));

  ctx.swapchain_ctx =
      WIND_TRY(swapchain::create(cfg, window.get_config().width, window.get_config().height, ctx.surface, ctx.device_ctx));

  // testing
  wind::ResourceManager manager{};
  auto                  texture = WIND_TRY(manager.load<wind::TextureHandle>("test.wind"));

  spdlog::info("got texture handle: idx: {}, generation: {}", texture.index, texture.generation);

  auto graphics_config = graphics::GraphicsConfig{.shader = {},
                                                  .rasterization{
                                                      .cull_mode    = CullMode::Back,
                                                      .polygon_mode = PolygonMode::Fill,
                                                      .front_face   = FrontFace::CounterClockwise,
                                                      .discard      = false,
                                                  },
                                                  .vertex_input_state{
                                                      .attributes{},
                                                      .bindings{},
                                                  },
                                                  .depth_stencil{
                                                      .depth_test = false,
                                                  },
                                                  .color_format = Format::RGBA8_SRGB};

  auto pipeline_info = WIND_TRY(graphics::create(ctx.device_ctx.device, std::move(graphics_config)));

  return ctx;
}

}  // namespace wind::vulkan
