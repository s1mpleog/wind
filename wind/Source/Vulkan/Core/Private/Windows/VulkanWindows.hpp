#pragma once

#include "Vulkan/Core/Private/VulkanGenericPlatform.h"

class FVulkanPlatformWindows : public FVulkanGenericPlatform
{
  public:
	static void GetInstanceExtensions(FVulkanInstanceExtensionArray &OutExtensions);
};

typedef FVulkanPlatformWindows FVulkanPlatform;
