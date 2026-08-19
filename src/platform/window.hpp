#pragma once

/* responsible to create platfrom agnostic window and expose functions for vulkan */

#include "SDL3/SDL_video.h"
#include "types.hpp"
#include "utils/expected_util.hpp"

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

inline auto make_window_config(const u16 width, const u16 height, std::string name) noexcept -> WindowConfiguration
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

  Window(Window&& other) noexcept
      : m_config{std::move(other.m_config)}
      , m_handle{std::exchange(other.m_handle, nullptr)} {};

  auto operator=(Window&& other) noexcept -> Window&
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

  [[nodiscard]] auto init() noexcept -> WindResult<void>;
  [[nodiscard]] auto extensions() const noexcept -> WindResult<std::vector<const char*>>;

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
