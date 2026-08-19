#include "context.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/core/instance.hpp"
#include "vulkan/core/validation_layer.hpp"

namespace wind::vulkan {
auto init(const platform::Window& window) noexcept -> WindResult<Context>
{
  Context ctx{};

  auto platform_extensions = WIND_TRY(window.extensions());
  ctx.instance             = WIND_TRY(create_instance(ctx.raii_ctx, std::move(platform_extensions)));

#ifdef WIND_VULKAN_VALIDATION
#ifdef WIND_LOG_ENABLE
  spdlog::info("WIND_VULKAN_VALIDATION enabled, setting up debug messenger");
#endif
  ctx.messenger = WIND_TRY(create_debug_utils(ctx.instance));
#endif

  return ctx;
}
}  // namespace wind::vulkan
