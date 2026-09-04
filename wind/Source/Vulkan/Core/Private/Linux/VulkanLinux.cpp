#include "VulkanLinux.hpp"

#include "Check.hpp"
#include "Config.hpp"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_vulkan.h"
#include "Vulkan/Core/Private/VulkanGenericPlatform.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"

#include <cstring>

void FVulkanPlatformLinux::GetInstanceExtensions(std::vector<const char *> &OutExtensions)
{
	// TODO: make sure SDL is init

	// temporary
	// if (!SDL_Init(SDL_INIT_VIDEO))
	// {
	// 	FATAL("Failed to initialized SDL3 Error code: {}", SDL_GetError());
	// }

	// i could use sdl_vulkan_get_instance_extensions but then i need sdl window handle so if i have to
	// pass it inside vulkan system and all i don't like it and since for now i am using gnu macros which is
	// not compatiable with msvc its very hard to run this project on non unix systems so hardcode linux only support
	// for now later if i want to port to win32 later i can add fvulkanwindows class or use
	// sdl_vulkan_getinstanceextensions

	const char *SDLDriver = SDL_GetCurrentVideoDriver();

	if (SDLDriver == nullptr)
	{
		FATAL("NOTE: `SDL_GetCurrentVideoDriver` return null either sdl is not initialized or something is wrong with "
		      "graphics environment");
	}

	if (strcmp(SDLDriver, "x11") == 0)
	{
		OutExtensions.emplace_back("VK_KHR_xlib_surface");
		return;
	}

	if (strcmp(SDLDriver, "wayland") == 0)
	{
		OutExtensions.emplace_back("VK_KHR_wayland_surface");
		return;
	}

	FATAL("Only X11 and Wayland display driver is supported on linux make sure your setup is correct if you are on "
	      "wayland only and having issues then try setting `export SDL_VIDEODRIVER=wayland` and run again");
}

void FVulkanPlatformLinux::GetDeviceExtensions(std::vector<const char *> &OutExtensions)
{
}

void FVulkanPlatformLinux::CreateSurface(FVulkanGenericPlatformWindowContext &WindowContext, vk::Instance Instance,
                                         vk::SurfaceKHR *OutSurface)
{
	// TODO: ensure sdl is init

	WIND_LOG(info, "{}", WindowContext.GetWindowHandle());

	WIND_ASSERT(WindowContext.GetWindowHandle() != nullptr &&
	            "Trying to create surface but WindowContext handle is nullptr");

	VkSurfaceKHR RawSurface = VK_NULL_HANDLE;
	if (SDL_Vulkan_CreateSurface(static_cast<SDL_Window *>(WindowContext.GetWindowHandle()), Instance, nullptr,
	                             &RawSurface) == false)
	{
		FATAL("SDL3 failed to create Vulkan Surface make sure graphic stack is setup correctly here is the sdl error "
		      "code for more information: {}",
		      SDL_GetError());
	}

	WIND_LOG(info, "[Platform Linux] SDL3 surface created successfully");

	*OutSurface = vk::SurfaceKHR{RawSurface};
}

void FVulkanPlatformLinux::DestroySurface(const vk::Instance Instance, vk::SurfaceKHR Surface)
{
	WIND_ASSERT(Surface != nullptr && "Cannot destroy surface because it's null");

	Instance.destroySurfaceKHR(Surface);
}
