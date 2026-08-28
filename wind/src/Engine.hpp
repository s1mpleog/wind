#pragma once

#include "Config.hpp"
#include "Input/InputManager.hpp"
#include "Platform/Window.hpp"
#include "Resources/ResourceManager.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Configuration.hpp"
#include "Vulkan/Core/Context.hpp"
#include "Vulkan/Graphics/PipelineManager.hpp"
#include "Vulkan/Renderer.hpp"
#include <memory>
#include "Scene/Scene.hpp"

class Engine
{
public:
  Engine(const Engine&)                    = delete;
  auto operator=(const Engine&) -> Engine& = delete;

  Engine(Engine&&) noexcept                    = default;
  auto operator=(Engine&&) noexcept -> Engine& = default;

  WIND_NODISCARD static auto create(WindowConfiguration window_cfg, Configuration vulkan_cfg) WIND_NOEXCEPT -> WindResult<Engine>;

  auto run() WIND_NOEXCEPT -> WindResult<void>;

private:
  Engine(Window                           window,
         std::unique_ptr<VulkanContext>   context,
         Renderer                         renderer,
         std::unique_ptr<InputManger>     input_manager,
         std::unique_ptr<ResourceManager> resource_manager,
         std::unique_ptr<PipelineManager> pipeline_manager,
         Scene                            scene)
      : m_window{std::move(window)}
      , m_vulkan_context{std::move(context)}
      , m_renderer{std::move(renderer)}
      , m_pipeline_manager{std::move(pipeline_manager)}
      , m_resource_manager{std::move(resource_manager)}
      , m_input_manager{std::move(input_manager)}
      , m_scene{std::move(scene)} {};

  Window                           m_window;
  std::unique_ptr<VulkanContext>   m_vulkan_context;
  Renderer                         m_renderer;
  std::unique_ptr<PipelineManager> m_pipeline_manager;
  std::unique_ptr<ResourceManager> m_resource_manager;
  std::unique_ptr<InputManger>     m_input_manager;
  Scene                            m_scene;
};
