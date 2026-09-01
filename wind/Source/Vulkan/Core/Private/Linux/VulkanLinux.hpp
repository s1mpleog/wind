#pragma once

#include "Vulkan/Core/Private/VulkanGenericPlatform.h"

class FVulkanPlatformLinux : public FVulkanGenericPlatform
{
  public:
	static void GetInstanceExtensions(FVulkanInstanceExtensionArray &OutExtensions);
};

typedef FVulkanPlatformLinux FVulkanPlatform;
