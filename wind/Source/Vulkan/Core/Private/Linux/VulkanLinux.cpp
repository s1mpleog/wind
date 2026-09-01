#include "VulkanLinux.hpp"

#include "Check.hpp"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_video.h"
#include "Vulkan/Core/Private/VulkanExtension.hpp"
#include "Vulkan/Core/Private/VulkanGenericPlatform.h"

#include <cstring>
#include <memory>

void FVulkanPlatformLinux::GetInstanceExtensions(FVulkanInstanceExtensionArray &OutExtensions)
{
	// TODO: make sure SDL is init

	// temporary
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		FATAL("Failed to initialized SDL3 Error code: {}", SDL_GetError());
	}

	// i could use sdl_vulkan_getinstanceextensions but then i need sdl window handle so if i have to
	// pass it inside vulkan system and all i don't like it and since for now i am using gnu macros which is
	// not compatiable with msvc its very hard to run this project on non unix systems so hardcode linux only support
	// for now later if i want to port to win32 later i can add fvulkanwindows class or use
	// sdl_vulkan_getinstanceextensions

	const char *SDLDriver = SDL_GetCurrentVideoDriver();

	if (SDLDriver == nullptr)
	{
		FATAL("NOTE: `SDL_GetCurrentVideoDriver` return null either sdl is not initialized or something wrong with "
		      "graphics environment");
	}

	if (strcmp(SDLDriver, "x11") == 0)
	{
		OutExtensions.emplace_back(std::make_unique<FVulkanInstanceExtension>("VK_KHR_xlib_surface", true));
		return;
	}

	if (strcmp(SDLDriver, "wayland") == 0)
	{
		OutExtensions.emplace_back(std::make_unique<FVulkanInstanceExtension>("VK_KHR_wayland_surface", true));
		return;
	}

	FATAL("Only X11 and Wayland display driver is supported on linux make sure your setup is correct if you are on "
	      "wayland only and having issues then try setting `export SDL_VIDEODRIVER=wayland` and run again");
}
