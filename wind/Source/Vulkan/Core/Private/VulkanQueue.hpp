#pragma once

#include "Config.hpp"
#include "vulkan/vulkan_core.h"

// vulkanDynamic.hpp -> exposes getter for most of the things like getDevice, getInstance, getExtensions etc
// VulkanState.hpp -> responsible to convert engine types to vulkan type ECompareFunction to vk::Compareop
// FVulkanGenericPlatform class -> is a bridge between platform and vulkan it exposes functions like
// Present, CreateSwapchain, create and destroy surface and it also exposes FVulkanGenericPlatformWindowContext
// which is underlying window abstraction it also exposes get instance and device extensions

class FVulkanDevice;

enum class EVulkanQueueType : uint8
{
	Graphics = 0,
	Transfer,
	Count
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

class FVulkanQueue
{
  public:
	FVulkanQueue(FVulkanDevice &InDevice, uint32 InFamilyIndex, EVulkanQueueType InQueueType);

	EVulkanQueueType GetQueueType() const
	{
		return QueueType;
	}

	uint32 GetQueueIndex() const
	{
		return QueueIndex;
	}

	uint32 GetFamilyIndex() const
	{
		return FamilyIndex;
	}

	vk::Queue &GetHandle()
	{
		return Queue;
	}

  private:
	uint32 FamilyIndex{};
	uint32 QueueIndex{};
	EVulkanQueueType QueueType;
	FVulkanDevice &Device;
	vk::Queue Queue{VK_NULL_HANDLE};
};
