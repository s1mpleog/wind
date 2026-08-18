#include "window.hpp"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_video.h"
#include "error.hpp"
#include "types.hpp"
#include <cassert>
#include <spdlog/spdlog.h>
#include "SDL3/SDL_vulkan.h"

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

#ifdef LOG_ENABLE
  spdlog::info("Window created: {}: {}", m_config.width, m_config.height);
#endif

  return {};
}


auto Window::extensions() const noexcept -> void
{

  u32 extensions_count{0};
  // extensions is a pointer to a const pointer to const char
  auto extensions = SDL_Vulkan_GetInstanceExtensions(&extensions_count);

  const char* y = "hello";

  // const char *
  // const *
  const char* const* x = &y;

  auto p = x;


  if(extensions_count == 0)
  {
    spdlog::error("sdl instance exension count is 0");
    return;
  }
}

}  // namespace wind::platform
