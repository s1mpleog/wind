#pragma once

#include "config.hpp"
#include "platform/window.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/core/configuration.hpp"
#include "vulkan/graphics/pipeline_manager.hpp"
#include "vulkan/renderer.hpp"

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
  Engine(platform::Window window, vulkan::Renderer renderer)
      : m_window{std::move(window)}
      , m_renderer{std::move(renderer)} {};

  platform::Window                  m_window;
  vulkan::Renderer                  m_renderer;
  vulkan::graphics::PipelineManager m_pipeline_manager;
};
};  // namespace wind
