#include "application.hpp"
#include "platform/window.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/core/context.hpp"
#include "vulkan/renderer.hpp"
#include <spdlog/spdlog.h>

namespace wind::app {
WIND_NODISCARD auto init(platform::WindowConfiguration win_cfg, vulkan::Configuration vulkan_cfg) WIND_NOEXCEPT -> WindResult<Context>
{
#ifdef WIND_LOG_ENABLE
  spdlog::info("initializing application...");
#endif

  auto window = platform::Window{std::move(win_cfg)};
  WIND_TRY(window.init());

  auto vulkan_context = WIND_TRY(vulkan::create_context(window, std::move(vulkan_cfg)));

  auto renderer = WIND_TRY(vulkan::Renderer::create(std::move(vulkan_context)));

  WIND_TRY(renderer.test());

#ifdef WIND_LOG_ENABLE
  spdlog::info("application init success");
#endif

  return Context{.window = std::move(window), .ctx = std::move(vulkan_context)};
}

}  // namespace wind::app
