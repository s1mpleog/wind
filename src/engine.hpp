#pragma once

#include "config.hpp"
#include "input/input_manager.hpp"
#include "platform/window.hpp"
#include "resources/resource_manager.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/core/configuration.hpp"
#include "vulkan/core/context.hpp"
#include "vulkan/graphics/pipeline_manager.hpp"
#include "vulkan/renderer.hpp"
#include <algorithm>
#include <memory>
#include "scene/scene.hpp"

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
