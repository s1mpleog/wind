#include "VulkanWindows.hpp"

#include "Check.hpp"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_vulkan.h"
#include "VulkanCheck.hpp"

void FVulkanPlatformWindows::GetInstanceExtensions(std::vector<const char *> &OutExtensions)
{
	OutExtensions.emplace_back("VK_KHR_win32_surface");
}

void FVulkanPlatformWindows::GetDeviceExtensions(std::vector<const char *> &OutExtensions)
{
}

// Testing just use SDL3 for now
// TODO: later replace it with win32 surface and handler
void FVulkanPlatformWindows::CreateSurface(FVulkanGenericPlatformWindowContext &WindowContext, vk::Instance Instance,
                                           vk::SurfaceKHR *OutSurface)
{
	// TODO: ensure sdl is init

	WIND_LOG(info, "{}", WindowContext.GetWindowHandle());

	assert(WindowContext.GetWindowHandle() != nullptr &&
	       "Trying to create surface but WindowContext handle is nullptr");

	VkSurfaceKHR RawSurface = VK_NULL_HANDLE;
	if (SDL_Vulkan_CreateSurface(static_cast<SDL_Window *>(WindowContext.GetWindowHandle()), Instance, nullptr,
	                             &RawSurface) == false)
	{
		FATAL("SDL3 failed to create Vulkan Surface make sure graphic stack is setup correctly here is the sdl error "
		      "code for more information: {}",
		      SDL_GetError());
	}

	WIND_LOG(info, "[Platform Windows] SDL3 surface created successfully");

	*OutSurface = vk::SurfaceKHR{RawSurface};
}

void FVulkanPlatformWindows::DestroySurface(const vk::Instance Instance, vk::SurfaceKHR Surface)
{
	if (Surface != VK_NULL_HANDLE)
	{
		Instance.destroySurfaceKHR(Surface);
	}
}
