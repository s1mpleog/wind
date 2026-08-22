#include "application.hpp"
#include "platform/window.hpp"
#include "types.hpp"
#include "vulkan/core/configuration.hpp"
#include <cstdlib>
#include <spdlog/spdlog.h>


auto main() -> i32
{
  constexpr u16 WINDOW_WIDTH  = 800;
  constexpr u16 WINDOW_HEIGHT = 600;

  auto window_cfg = wind::platform::WindowConfiguration{.name = "Wind", .width = WINDOW_WIDTH, .height = WINDOW_HEIGHT};
  auto vulkan_cfg = wind::vulkan::presets::Development;
  vulkan_cfg.app_name = "Wind";

  auto app = wind::Application::create(std::move(window_cfg), std::move(vulkan_cfg));

  if(!app)
  {
    spdlog::error("{}", app.error().to_string());
    return EXIT_FAILURE;
  }

  if(!app->run())
  {
    spdlog::error("{}", app.error().to_string());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
