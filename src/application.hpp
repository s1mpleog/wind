#pragma once

#include "config.hpp"
#include "platform/window.hpp"
#include "resources/resource_manager.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/core/configuration.hpp"
#include "vulkan/renderer.hpp"

namespace wind {

class Application
{
public:
  Application(const Application&)                    = delete;
  auto operator=(const Application&) -> Application& = delete;

  Application(Application&&) noexcept                    = default;
  auto operator=(Application&&) noexcept -> Application& = default;

  WIND_NODISCARD static auto create(platform::WindowConfiguration window_cfg, wind::vulkan::Configuration vulkan_cfg) WIND_NOEXCEPT
      -> WindResult<Application>;

  auto run() WIND_NOEXCEPT -> WindResult<void>;

private:
  Application(platform::Window window, vulkan::Renderer renderer)
      : m_window{std::move(window)}
      , m_renderer{std::move(renderer)} {};

  platform::Window m_window;
  vulkan::Renderer m_renderer;
  ResourceManager  m_resources;
};

};  // namespace wind
