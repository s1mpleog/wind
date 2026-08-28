#pragma once

/* responsible to create platfrom agnostic window and expose functions for vulkan */

#include "SDL3/SDL_video.h"
#include "Config.hpp"
#include "./Types.hpp"
#include "utils/ExpectedUtil.hpp"
#include "vulkan/vulkan.hpp"

#include <string>
#include <utility>

#include <spdlog/spdlog.h>
#include <vector>

namespace wind::platform {

struct WindowConfiguration
{
  std::string name;
  u16         width{};
  u16         height{};
};

inline auto make_window_config(const u16 width, const u16 height, std::string name) WIND_NOEXCEPT -> WindowConfiguration
{
  return WindowConfiguration{.name = std::move(name), .width = width, .height = height};
}

class Window
{
public:
  explicit Window(WindowConfiguration cfg)
      : m_config{std::move(cfg)} {};

  Window(const Window&)                   = delete;
  auto operator=(const Window&) -> Window = delete;

  Window(Window&& other) WIND_NOEXCEPT : m_config{std::move(other.m_config)}, m_handle{std::exchange(other.m_handle, nullptr)} {};

  auto operator=(Window&& other) WIND_NOEXCEPT->Window&
  {
    if(this != &other)
    {
      if(m_handle != nullptr)
        SDL_DestroyWindow(m_handle);

      m_handle = other.m_handle;
      m_config = std::move(other.m_config);

      other.m_config = {};
      other.m_handle = nullptr;
    }

    return *this;
  };

  WIND_NODISCARD auto create() WIND_NOEXCEPT -> WindResult<void>;
  WIND_NODISCARD auto extensions() const WIND_NOEXCEPT -> WindResult<std::vector<const char*>>;
  WIND_NODISCARD auto create_surface(const vk::Instance& instance) const WIND_NOEXCEPT -> WindResult<VkSurfaceKHR>;
  auto                get_config() const WIND_NOEXCEPT -> const WindowConfiguration&;

  WIND_NODISCARD auto drawable_size() const WIND_NOEXCEPT -> std::pair<u32, u32>
  {
    return {m_config.width, m_config.height};
  }

  WIND_NODISCARD auto handle() WIND_NOEXCEPT -> SDL_Window* { return m_handle; }

  ~Window()
  {
    if(m_handle != nullptr)
    {
      SDL_DestroyWindow(m_handle);
#ifdef WIND_LOG_ENABLE
      spdlog::info("window handler destroyed");
#endif
    }
  }

private:
  WindowConfiguration m_config{};
  SDL_Window*         m_handle{nullptr};
};

}  // namespace wind::platform
