#pragma once

#include "Config.hpp"
#include "input/InputManager.hpp"
#include "platform/Window.hpp"
#include "resources/ResourceManager.hpp"
#include "utils/ExpectedUtil.hpp"
#include "vulkan/core/Configuration.hpp"
#include "vulkan/core/Context.hpp"
#include "vulkan/graphics/PipelineManager.hpp"
#include "vulkan/Renderer.hpp"
#include <memory>
#include "scene/Scene.hpp"

namespace wind {
class Engine
{
public:
  Engine(const Engine&)                    = delete;
  auto operator=(const Engine&) -> Engine& = delete;

  Engine(Engine&&) noexcept                    = default;
  auto operator=(Engine&&) noexcept -> Engine& = default;

  WIND_NODISCARD static auto create(platform::WindowConfiguration window_cfg, wind::vulkan::Configuration vulkan_cfg) WIND_NOEXCEPT
      -> WindResult<Engine>;

  auto run() WIND_NOEXCEPT -> WindResult<void>;

private:
  Engine(platform::Window                                   window,
         std::unique_ptr<vulkan::VulkanContext>             context,
         vulkan::Renderer                                   renderer,
         std::unique_ptr<input::InputManger>                input_manager,
         std::unique_ptr<resources::ResourceManager>        resource_manager,
         std::unique_ptr<vulkan::graphics::PipelineManager> pipeline_manager,
         scene::Scene                                       scene)
      : m_window{std::move(window)}
      , m_vulkan_context{std::move(context)}
      , m_renderer{std::move(renderer)}
      , m_pipeline_manager{std::move(pipeline_manager)}
      , m_resource_manager{std::move(resource_manager)}
      , m_input_manager{std::move(input_manager)}
      , m_scene{std::move(scene)} {};

  platform::Window                                   m_window;
  std::unique_ptr<vulkan::VulkanContext>             m_vulkan_context;
  vulkan::Renderer                                   m_renderer;
  std::unique_ptr<vulkan::graphics::PipelineManager> m_pipeline_manager;
  std::unique_ptr<resources::ResourceManager>        m_resource_manager;
  std::unique_ptr<input::InputManger>                m_input_manager;
  scene::Scene                                       m_scene;
};

};  // namespace wind
