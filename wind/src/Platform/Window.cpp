#include "Platform/Window.hpp"

#include "Error.hpp"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_vulkan.h"
#include "Types.hpp"
#include "Utils/ExpectedUtil.hpp"

#include <cassert>
#include <spdlog/spdlog.h>
#include <vector>

WIND_NODISCARD auto FUWindow::Create() WIND_NOEXCEPT -> TWindResult<void>
{
	if (MConfig.Width == 0 || MConfig.Height == 0 || MConfig.Name.empty())
	{
		WIND_ERR(WindError::internal(ErrorCode::InvalidWindowConfig));
	}

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		WIND_ERR(WindError::sdl(ErrorCode::FailedToInitSDL));
	}

	MHandle = SDL_CreateWindow(MConfig.Name.c_str(), MConfig.Width, MConfig.Height,
	                           SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

	if (MHandle == nullptr)
	{
		WIND_ERR(WindError::sdl(ErrorCode::FailedToCreateWindow));
	}

	SDL_SetWindowRelativeMouseMode(MHandle, true);

#ifdef WIND_LOG_ENABLE
	spdlog::info("Window created: {}x{}", MConfig.Width, MConfig.Height);
#endif

	return {};
}

WIND_NODISCARD auto FUWindow::Extensions() const WIND_NOEXCEPT -> TWindResult<std::vector<const char *>>
{
	WIND_ASSERT(MHandle != nullptr && "window handler is nullptr");

	TU32 ExtensionsCount{0};
	// extensions is a pointer to a const pointer to const char
	const auto *const ExtensionsRaw = SDL_Vulkan_GetInstanceExtensions(&ExtensionsCount);

	if (ExtensionsCount == 0)
	{
		WIND_ERR(WindError::internal(ErrorCode::ExtensionNotSupported));
	}

	std::vector<const char *> Extensions;
	Extensions.reserve(ExtensionsCount);

	for (TUsize I = 0; I < ExtensionsCount; ++I)
	{
		Extensions.emplace_back(ExtensionsRaw[I]);
	}

#ifdef WIND_LOG_ENABLE
	spdlog::info("SDL3 returns {} extensions", ExtensionsCount);
	std::ranges::for_each(Extensions, [](auto Extension) -> auto { spdlog::info("extension: {}", Extension); });
#endif

	return Extensions;
}

WIND_NODISCARD auto FUWindow::CreateSurface(const vk::Instance &Instance) const WIND_NOEXCEPT -> TWindResult<VkSurfaceKHR>
{
	WIND_ASSERT(MHandle != nullptr && "trying to create vulkan surface but window handle is null");

	VkSurfaceKHR Surface{};

	if (!SDL_Vulkan_CreateSurface(MHandle, Instance, nullptr, &Surface))
		WIND_ERR(WindError::sdl(ErrorCode::FailedToCreateSurface));

#ifdef WIND_LOG_ENABLE
	spdlog::info("successfully created vulkan surface");
#endif

	return Surface;
}

auto FUWindow::GetConfig() const WIND_NOEXCEPT -> const FWindowConfiguration &
{
	return MConfig;
}
