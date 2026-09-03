#pragma once

#include "Config.hpp"
#include "Vulkan/Core/Private/Linux/VulkanLinux.hpp"
#include "vulkan/vulkan_core.h"

#include <vector>
#include <vulkan/vulkan_core.h>

static WIND_INLINE std::vector<const char *> GetWindInstanceExtensions()
{
	std::vector<const char *> WindExtensions;
	WindExtensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef WIND_VULKAN_VALIDATION
	WindExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	FVulkanPlatform::GetInstanceExtensions(WindExtensions);

	return WindExtensions;
}

static WIND_INLINE std::vector<const char *> GetWindDeviceExtensions()
{
	std::vector<const char *> WindExtensions;

	WindExtensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	WindExtensions.emplace_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
	WindExtensions.emplace_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);

	FVulkanPlatform::GetDeviceExtensions(WindExtensions);

	return WindExtensions;
}
