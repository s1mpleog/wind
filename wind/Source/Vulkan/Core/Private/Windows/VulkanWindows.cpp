#include "VulkanWindows.hpp"

#include "Vulkan/Core/Private/VulkanExtension.hpp"
#include "Vulkan/Core/Private/VulkanGenericPlatform.h"

#include <memory>

void FVulkanPlatformWindows::GetInstanceExtensions(FVulkanInstanceExtensionArray &OutExtensions)
{
	OutExtensions.emplace_back(std::make_unique<FVulkanInstanceExtension>("VK_KHR_win32_surface", true));
}
