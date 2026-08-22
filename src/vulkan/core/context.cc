#include "context.hpp"
#include "resources/resource_manager.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/core/device.hpp"
#include "vulkan/frame/frame_context.hpp"
#include "vulkan/core/instance.hpp"
#include "vulkan/core/validation_layer.hpp"
#include "swapchain.hpp"
#include "vulkan/graphics/pipeline.hpp"

namespace wind::vulkan {
auto create(const platform::Window& window, Configuration cfg) WIND_NOEXCEPT -> WindResult<Context>
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

  ctx.frame_context.reserve(MAX_FRAME_IN_FLIGHT);

  ctx.frame_context =
      (WIND_TRY(frame::create(MAX_FRAME_IN_FLIGHT, ctx.device_ctx.device, ctx.device_ctx.graphics_pool, nullptr)));

  wind::ResourceManager manager{};
  auto                  texture = WIND_TRY(manager.load<wind::TextureHandle>("test.wind"));

  spdlog::info("got texture handle: idx: {}, generation: {}", texture.index, texture.generation);

  auto pipeline = WIND_TRY(graphics::create(ctx.device_ctx.device));

  return ctx;
}

}  // namespace wind::vulkan
