#include "renderer.hpp"
#include "resources/resource_manager.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/core/context.hpp"
#include "vulkan/frame/frame_context.hpp"

namespace wind::vulkan {

WIND_NODISCARD auto Renderer::create(Context context) WIND_NOEXCEPT -> WindResult<Renderer>
{
  auto frame_context = WIND_TRY(frame::create(MAX_FRAME_IN_FLIGHT, context.device_ctx.device, context.device_ctx.graphics_pool));

  return Renderer(std::move(context), std::move(frame_context));
}

}  // namespace wind::vulkan
