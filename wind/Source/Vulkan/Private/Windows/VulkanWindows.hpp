#pragma once

#include "VulkanGenericPlatform.h"

class FVulkanPlatformWindows : public FVulkanGenericPlatform
{
  public:
	static void GetInstanceExtensions(std::vector<const char *> &OutExtensions);
	static void GetDeviceExtensions(std::vector<const char *> &OutExtensions);
	static void CreateSurface(FVulkanGenericPlatformWindowContext &WindowContext, vk::Instance Instance,
	                          vk::SurfaceKHR *OutSurface);

	static void DestroySurface(const vk::Instance Instance, vk::SurfaceKHR Surface);
};

typedef FVulkanPlatformWindows FVulkanPlatform;
