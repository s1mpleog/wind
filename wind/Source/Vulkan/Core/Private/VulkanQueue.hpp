#pragma once

#include "Config.hpp"

// vulkanDynamic.hpp -> exposes getter for most of the things like getDevice, getInstance, getExtensions etc
// VulkanState.hpp -> responsible to convert engine types to vulkan type ECompareFunction to vk::Compareop
// FVulkanGenericPlatform class -> is a bridge between platform and vulkan it exposes functions like
// Present, CreateSwapchain, create and destroy surface and it also exposes FVulkanGenericPlatformWindowContext
// which is underlying window abstraction it also exposes get instance and device extensions

enum class EVulkanQueueType : uint8
{
	Graphics = 0,
	Transfer
};

WIND_INLINE const char *GetVulkanQueueTypeName(EVulkanQueueType QueueType)
{
	switch (QueueType)
	{
	case EVulkanQueueType::Graphics:
		return "Graphics";
	case EVulkanQueueType::Transfer:
		return "Transfer";

	default:
		return "Invalid";
	}
}
