#pragma once

#include "Vulkan/Core/Private/VulkanGenericPlatform.h"
#include "vulkan/vulkan_raii.hpp"

class FVulkanPlatformLinux : public FVulkanGenericPlatform
{
  public:
	static void GetInstanceExtensions(FVulkanInstanceExtensionArray &OutExtensions);
	static void CreateSurface(FVulkanGenericPlatformWindowContext &WindowContext, const vk::raii::Instance &Instance,
	                          vk::SurfaceKHR *OutSurface);

	static void DestroySurface(vk::raii::Instance &Instance, vk::SurfaceKHR Surface);
};

typedef FVulkanPlatformLinux FVulkanPlatform;
