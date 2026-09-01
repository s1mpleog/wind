#pragma once

#include "vulkan/vulkan_core.h"

#include <vector>

using FVulkanDeviceExtensionArray = std::vector<std::unique_ptr<class FVulkanDeviceExtension>>;
using FVulkanInstanceExtensionArray = std::vector<std::unique_ptr<class FVulkanInstanceExtension>>;

// TODO: can we avoid void here and handle it in better way?
class FVulkanGenericPlatformWindowContext
{
  public:
	FVulkanGenericPlatformWindowContext(void *InWindowHandle) : WindowHandle(InWindowHandle) {};

	void *GetWindowHandle() const
	{
		return WindowHandle;
	}

  private:
	void *WindowHandle = nullptr;
};

class FVulkanGenericPlatform
{
  public:
	static void GetInstanceExtensions(FVulkanInstanceExtensionArray &OutExtensions);
	static void GetDeviceExtensions(FVulkanDeviceExtensionArray &OutExtensions);

	static void CreateSurface(vk::SurfaceKHR *OutSurface);
	static void DestroySurface(vk::raii::Instance &Instance, vk::SurfaceKHR Surface);

	static void Present(vk::Queue Queue, vk::PresentInfoKHR &PresentInfo);

	// todo: handle this i need window context that should need to platform and library agnostic
	// should not have sdl types or any platform type
	static vk::Result CreateSwapchainKHR();
	static void DestroySwapchain();
};
