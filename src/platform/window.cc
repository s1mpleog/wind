#include "window.hpp"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_video.h"
#include "error.hpp"
#include "types.hpp"
#include <cassert>
#include <spdlog/spdlog.h>
#include <vector>
#include "SDL3/SDL_vulkan.h"
#include "utils/expected_util.hpp"

namespace wind::platform {
[[nodiscard]] auto Window::init() noexcept -> WindResult<void>
{
  if(m_config.width == 0 || m_config.height == 0 || m_config.name.empty())
  {
    return std::unexpected(WindError::internal(ErrorCode::InvalidWindowConfig));
  }

  if(!SDL_Init(SDL_INIT_VIDEO))
  {
    return std::unexpected(WindError::sdl(ErrorCode::FailedToInitSDL));
  }

  m_handle = SDL_CreateWindow(m_config.name.c_str(), m_config.width, m_config.height,
                              SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

  if(m_handle == nullptr)
  {
    return std::unexpected(WindError::sdl(ErrorCode::FailedToCreateWindow));
  }

#ifdef WIND_LOG_ENABLE
  spdlog::info("Window created: {}x{}", m_config.width, m_config.height);
#endif

  return {};
}

[[nodiscard]] auto Window::extensions() const noexcept -> WindResult<std::vector<const char*>>
{
  assert(m_handle != nullptr && "window handler is nullptr");

  u32 extensions_count{0};
  // extensions is a pointer to a const pointer to const char
  const auto* const extensions_raw = SDL_Vulkan_GetInstanceExtensions(&extensions_count);

  if(extensions_count == 0)
  {
    return std::unexpected(WindError::internal(ErrorCode::ExtensionNotSupported));
  }

  std::vector<const char*> extensions(extensions_count);

  for(usize i = 0; i < extensions_count; ++i)
  {
    extensions[i] = extensions_raw[i];
  }

#ifdef WIND_LOG_ENABLE
  spdlog::info("SDL3 returns {} extensions", extensions_count);
  std::ranges::for_each(extensions, [](auto extension) -> auto { spdlog::info("extension: {}", extension); });
#endif

  return extensions;
}

}  // namespace wind::platform
