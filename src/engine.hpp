#pragma once

#include "config.hpp"
#include "input/input_manager.hpp"
#include "platform/window.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/core/configuration.hpp"
#include "vulkan/graphics/pipeline_manager.hpp"
#include "vulkan/renderer.hpp"
#include <memory>

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
  Engine(platform::Window window, vulkan::Renderer renderer, std::unique_ptr<input::InputManger> input_manager)
      : m_window{std::move(window)}
      , m_renderer{std::move(renderer)}
      , m_input_manager{std::move(input_manager)} {};

  platform::Window                  m_window;
  vulkan::Renderer                  m_renderer;
  vulkan::graphics::PipelineManager m_pipeline_manager;
  // painful but have to do it thanks to std::expected :(
  std::unique_ptr<input::InputManger> m_input_manager;
};

};  // namespace wind
