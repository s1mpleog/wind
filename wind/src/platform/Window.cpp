#include "Window.hpp"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_video.h"
#include "./Error.hpp"
#include "./Types.hpp"
#include <cassert>
#include <spdlog/spdlog.h>
#include <vector>
#include "SDL3/SDL_vulkan.h"
#include "utils/ExpectedUtil.hpp"

namespace wind::platform {
WIND_NODISCARD auto Window::create() WIND_NOEXCEPT -> WindResult<void>
{
  if(m_config.width == 0 || m_config.height == 0 || m_config.name.empty())
  {
    WIND_ERR(WindError::internal(ErrorCode::InvalidWindowConfig));
  }

  if(!SDL_Init(SDL_INIT_VIDEO))
  {
    WIND_ERR(WindError::sdl(ErrorCode::FailedToInitSDL));
  }

  m_handle = SDL_CreateWindow(m_config.name.c_str(), m_config.width, m_config.height,
                              SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

  if(m_handle == nullptr)
  {
    WIND_ERR(WindError::sdl(ErrorCode::FailedToCreateWindow));
  }

  SDL_SetWindowRelativeMouseMode(m_handle, true);

#ifdef WIND_LOG_ENABLE
  spdlog::info("Window created: {}x{}", m_config.width, m_config.height);
#endif

  return {};
}

WIND_NODISCARD auto Window::extensions() const WIND_NOEXCEPT -> WindResult<std::vector<const char*>>
{
  WIND_ASSERT(m_handle != nullptr && "window handler is nullptr");

  u32 extensions_count{0};
  // extensions is a pointer to a const pointer to const char
  const auto* const extensions_raw = SDL_Vulkan_GetInstanceExtensions(&extensions_count);

  if(extensions_count == 0)
  {
    WIND_ERR(WindError::internal(ErrorCode::ExtensionNotSupported));
  }

  std::vector<const char*> extensions;
  extensions.reserve(extensions_count);

  for(usize i = 0; i < extensions_count; ++i)
  {
    extensions.emplace_back(extensions_raw[i]);
  }

#ifdef WIND_LOG_ENABLE
  spdlog::info("SDL3 returns {} extensions", extensions_count);
  std::ranges::for_each(extensions, [](auto extension) -> auto { spdlog::info("extension: {}", extension); });
#endif

  return extensions;
}

WIND_NODISCARD auto Window::create_surface(const vk::Instance& instance) const WIND_NOEXCEPT -> WindResult<VkSurfaceKHR>
{
  WIND_ASSERT(m_handle != nullptr && "trying to create vulkan surface but window handle is null");

  VkSurfaceKHR surface{};

  if(!SDL_Vulkan_CreateSurface(m_handle, instance, nullptr, &surface))
    WIND_ERR(WindError::sdl(ErrorCode::FailedToCreateSurface));

#ifdef WIND_LOG_ENABLE
  spdlog::info("successfully created vulkan surface");
#endif

  return surface;
}

auto Window::get_config() const WIND_NOEXCEPT -> const WindowConfiguration&
{
  return m_config;
}

}  // namespace wind::platform
