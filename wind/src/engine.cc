#include "engine.hpp"
#include "SDL3/SDL_events.h"
#include "core/service_locator.hpp"
#include "error.hpp"
#include "input/input_manager.hpp"
#include "platform/window.hpp"
#include "resources/builtin.hpp"
#include "resources/resource_manager.hpp"
#include "scene/render_object.hpp"
#include "scene/scene.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/core/context.hpp"
#include "vulkan/graphics/pipeline_manager.hpp"
#include "vulkan/renderer.hpp"
#include <memory>
#include <spdlog/spdlog.h>
#include <SDL3/SDL_timer.h>

namespace wind {
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

  // load all the models and pipelines
  auto assets = WIND_TRY(builtin::build(resource_manager.get(), pipeline_manager.get(), vulkan_context->gpu_device.device));

  scene::Scene scene{};

  for(const auto& asset : assets)
  {
    scene.add_render_objects(asset, asset.is_model);
  }

  auto renderer = WIND_TRY(vulkan::Renderer::create(vulkan_cfg, window, vulkan_context.get(), resource_manager.get(),
                                                    pipeline_manager.get()));

#ifdef WIND_LOG_ENABLE
  spdlog::info("Engine created successfully");
#endif

  return Engine(std::move(window), std::move(vulkan_context), std::move(renderer), std::move(input_manager),
                std::move(resource_manager), std::move(pipeline_manager), std::move(scene));
}

auto Engine::run() WIND_NOEXCEPT -> WindResult<void>
{
  bool running = true;

  [[maybe_unused]] float time = 0.0F;

  u64 last = SDL_GetPerformanceCounter();

  //TODO: abstract this
  while(running)
  {
    uint64_t now   = SDL_GetPerformanceCounter();
    float    delta = static_cast<float>(now - last) / static_cast<float>(SDL_GetPerformanceFrequency());
    last           = now;
    time += delta;

    SDL_Event event{};

    m_input_manager->begin_frame();

    while(SDL_PollEvent(&event))
    {
      m_input_manager->process_event(event);


      if(event.type == SDL_EVENT_WINDOW_RESIZED)
      {
        int new_width  = event.window.data1;
        int new_height = event.window.data2;
        // Handle new dimensions here
        break;
      }

      if(event.type == SDL_EVENT_QUIT)
        running = false;
    }

    // TODO: do not use this
    // m_input_manager->update();

    int width{};
    int height{};

    SDL_GetWindowSizeInPixels(m_window.handle(), &width, &height);

    m_scene.camera.update_aspect(width, height);

    m_scene.camera.process_mouse();
    m_scene.camera.process_keyboard(delta);

    auto begin_result = m_renderer.begin(static_cast<u32>(width), static_cast<u32>(height));

    if(!begin_result)
    {
      if(begin_result.error().code == ErrorCode::SwapchainOutOfDate)
        continue;

      WIND_ERR(begin_result.error());
    }

    auto camera_view = m_scene.camera.render_view();

    for(const auto& object : m_scene.get())
    {
      m_renderer.draw(object, camera_view);
    }

    m_renderer.end();
  }

  WIND_TRY(m_renderer.shutdown());

  return {};
}

}  // namespace wind
