#include "window.hpp"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_video.h"
#include "error.hpp"
#include <X11/X.h>
#include <cassert>

namespace wind::platform {
auto Window::init(WindowConfiguration config) -> WindResult<Window>
{

  if(config.width == 0 || config.height == 0 || config.name.empty())
  {
    return std::unexpected(WindError::internal(ErrorCode::InvalidWindowConfig));
  }

  if(!SDL_Init(SDL_INIT_VIDEO))
  {
    return std::unexpected(WindError::sdl(ErrorCode::FailedToInitSDL));
  }

  Window window{};
  window.m_config = std::move(config);

  window.m_handle = SDL_CreateWindow(window.m_config.name.c_str(), window.m_config.width, window.m_config.height,
                                     SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

  if(window.m_handle == nullptr)
  {
    return std::unexpected(WindError::sdl(ErrorCode::FailedToCreateWindow));
  }

  return window;
}

}  // namespace wind::platform
