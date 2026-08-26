#include "engine.hpp"
#include "SDL3/SDL_events.h"
#include "core/service_locator.hpp"
#include "error.hpp"
#include "input/input_manager.hpp"
#include "platform/window.hpp"
#include "resources/resource_manager.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/core/context.hpp"
#include "vulkan/graphics/pipeline_manager.hpp"
#include "vulkan/renderer.hpp"
#include <memory>
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

  auto input_manager = std::make_unique<input::InputManger>(input::InputManger{});
  core::ServiceLocator::provide(input_manager.get());

  auto vulkan_context = std::make_unique<vulkan::VulkanContext>(WIND_TRY(vulkan::create_context(window, vulkan_cfg)));

  auto resource_manager =
      std::make_unique<resources::ResourceManager>(WIND_TRY(resources::ResourceManager::create(vulkan_context.get())));

  auto pipeline_manager = std::make_unique<vulkan::graphics::PipelineManager>(vulkan::graphics::PipelineManager{});

  auto renderer = WIND_TRY(vulkan::Renderer::create(std::move(vulkan_cfg), window, vulkan_context.get(),
                                                    resource_manager.get(), pipeline_manager.get()));

  // temporary
  WIND_TRY(renderer.initialize_resources());

#ifdef WIND_LOG_ENABLE
  spdlog::info("Engine created successfully");
#endif

  return Engine(std::move(window), std::move(vulkan_context), std::move(renderer), std::move(input_manager),
                std::move(resource_manager), std::move(pipeline_manager));
}

auto Engine::run() WIND_NOEXCEPT -> WindResult<void>
{
  bool running = true;

  //TODO: abstract this
  while(running)
  {
    SDL_Event event{};

    m_input_manager->update();

    while(SDL_PollEvent(&event))
    {
      if(event.type == SDL_EVENT_QUIT)
        running = false;

      //TODO:
      // m_process_event(event)
    }

    m_input_manager->update();


    int width{};
    int height{};

    SDL_GetWindowSizeInPixels(m_window.handle(), &width, &height);

    auto begin_result = m_renderer.begin(static_cast<u32>(width), static_cast<u32>(height));

    if(!begin_result)
    {
      if(begin_result.error().code == ErrorCode::SwapchainOutOfDate)
        continue;

      spdlog::info("error: {}", begin_result.error().to_string());
      WIND_ERR(begin_result.error());
    }

    m_renderer.draw();

    m_renderer.end();
  }

  WIND_TRY(m_renderer.shutdown());

  return {};
}

}  // namespace wind
