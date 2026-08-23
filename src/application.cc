#include "application.hpp"
#include "SDL3/SDL_events.h"
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
  bool running = true;

  //TODO: abstract this
  while(running)
  {
    SDL_Event event{};

    while(SDL_PollEvent(&event))
    {
      if(event.type == SDL_EVENT_QUIT)
        running = false;
    }

    auto [width, height] = m_window.drawable_size();

    WIND_TRY(m_renderer.begin(width, height));

    m_renderer.draw();

    m_renderer.end();
  }

  WIND_TRY(m_renderer.shutdown());

  return {};
}

}  // namespace wind
