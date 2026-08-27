#include "engine.hpp"
#include "platform/window.hpp"
#include "types.hpp"
#include "vulkan/core/configuration.hpp"
#include <cstdlib>
#include <spdlog/spdlog.h>


auto main() -> i32
{
  constexpr u16 WINDOW_WIDTH  = 1280;
  constexpr u16 WINDOW_HEIGHT = 720;

  auto window_cfg = wind::platform::WindowConfiguration{.name = "Wind", .width = WINDOW_WIDTH, .height = WINDOW_HEIGHT};
  auto vulkan_cfg = wind::vulkan::presets::Default;
  vulkan_cfg.app_name = "Wind";

  auto engine = wind::Engine::create(std::move(window_cfg), std::move(vulkan_cfg));

  if(!engine)
  {
    spdlog::error("{}", engine.error().to_string());
    return EXIT_FAILURE;
  }

  if(!engine->run())
  {
    spdlog::error("{}", engine.error().to_string());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
