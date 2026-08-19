#include "application.hpp"
#include "platform/window.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/core/context.hpp"
#include <spdlog/spdlog.h>

namespace wind::app {
[[nodiscard]] auto init(platform::WindowConfiguration win_cfg, vulkan::Configuration vulkan_cfg) noexcept -> WindResult<Context>
{
#ifdef WIND_LOG_ENABLE
  spdlog::info("initializing application...");
#endif

  auto window = platform::Window{std::move(win_cfg)};
  WIND_TRY(window.init());

  auto vulkan = WIND_TRY(vulkan::init(window, std::move(vulkan_cfg)));

#ifdef WIND_LOG_ENABLE
  spdlog::info("application init success");
#endif

  return Context{.window = std::move(window), .ctx = std::move(vulkan)};
}

}  // namespace wind::app
