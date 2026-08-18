#pragma once

/* responsible to create platfrom agnostic window and expose functions for vulkan */

#include "SDL3/SDL_video.h"
#include "types.hpp"
#include "utils/expected_util.hpp"

#include <string>
#include <utility>

namespace wind::platform {

struct WindowConfiguration
{
  std::string name;
  u16         width{};
  u16         height{};
};

struct Window
{
  WindowConfiguration m_config{};
  SDL_Window*         m_handle{nullptr};

  Window() = default;

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


  auto init(WindowConfiguration config) -> WindResult<Window>;

  ~Window()
  {
    if(m_handle != nullptr)
    {
      SDL_DestroyWindow(m_handle);
    }
  }
};


}  // namespace wind::platform
