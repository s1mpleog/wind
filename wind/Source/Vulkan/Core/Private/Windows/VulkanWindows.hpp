#pragma once

#include "Vulkan/Core/Private/VulkanGenericPlatform.h"

class FVulkanPlatformWindows : public FVulkanGenericPlatform
{
  public:
	static void GetInstanceExtensions(std::vector<const char *> &OutExtensions);
	static void GetDeviceExtensions(std::vector<const char *> &OutExtensions);
};

typedef FVulkanPlatformWindows FVulkanPlatform;
