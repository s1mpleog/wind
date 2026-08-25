#include "engine.hpp"
#include "SDL3/SDL_events.h"
#include "platform/window.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/renderer.hpp"
#include <spdlog/spdlog.h>

namespace wind {

//current life-cycle is following:
// at lower level vulkan::context owns instance, debug messenger, GpuDevice and Swapchain
// FrameContext owns N semaphores, fences and cmd buffers
// Renderer owns vulkan::context and Frame context
// Window owns its internal handler nothing else
// Engine owns window and renderer
// main createsEngine
// Renderer -> frame context and vulkan context
// Engine -> Window and Renderer
// main -> callsEngine

WIND_NODISCARD auto Engine::create(platform::WindowConfiguration window_cfg, wind::vulkan::Configuration vulkan_cfg) WIND_NOEXCEPT
    -> WindResult<Engine>
{
#ifdef WIND_LOG_ENABLE
  spdlog::info("initializing Engine...");
#endif

  auto window = platform::Window{std::move(window_cfg)};
  WIND_TRY_VOID(window.create());

  // let renderer the subsystems for render
  auto renderer = WIND_TRY(vulkan::Renderer::create(std::move(vulkan_cfg), window));
  // load textures, build pipelines, create shaders and all
  WIND_TRY(renderer.initialize_resources());

#ifdef WIND_LOG_ENABLE
  spdlog::info("Engine created successfully");
#endif

  return Engine(std::move(window), std::move(renderer));
}

auto Engine::run() WIND_NOEXCEPT -> WindResult<void>
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
