#include "Vulkan/Core/Context.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Device.hpp"
#include "Vulkan/Core/Instance.hpp"
#include "Vulkan/Core/ValidationLayer.hpp"

namespace wind::vulkan {
auto create_context(const platform::Window& window, const Configuration& cfg) WIND_NOEXCEPT -> WindResult<VulkanContext>
{
  VulkanContext ctx{};

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

  ctx.gpu_device = WIND_TRY(device::create(cfg, ctx.instance, ctx.surface));

  return ctx;
}

}  // namespace wind::vulkan
