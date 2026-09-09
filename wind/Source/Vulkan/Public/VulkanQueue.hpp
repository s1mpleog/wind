#pragma once

// #include "Config.hpp"
#include "VulkanCommandBuffer.hpp"
#include "vulkan/vulkan_core.h"

#include <array>
#include <memory>

#include <cstddef>
#include <cstdint>

// vulkanDynamic.hpp -> exposes getter for most of the things like getDevice, getInstance, getExtensions etc
// VulkanState.hpp -> responsible to convert engine types to vulkan type ECompareFunction to vk::Compareop
// FVulkanGenericPlatform class -> is a bridge between platform and vulkan it exposes functions like
// Present, CreateSwapchain, create and destroy surface and it also exposes FVulkanGenericPlatformWindowContext
// which is underlying window abstraction it also exposes get instance and device extensions

class FVulkanDevice;
class FVulkanCommandBufferPool;

enum class EVulkanQueueType : uint8_t
{
	Graphics = 0,
	Transfer,
	Count
};

inline const char *GetVulkanQueueTypeName(EVulkanQueueType QueueType)
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
	FVulkanQueue(FVulkanDevice &InDevice, uint32_t InFamilyIndex, EVulkanQueueType InQueueType);

	EVulkanQueueType GetQueueType() const
	{
		return QueueType;
	}

	uint32_t GetQueueIndex() const
	{
		return QueueIndex;
	}

	uint32_t GetFamilyIndex() const
	{
		return FamilyIndex;
	}

	vk::Queue &GetHandle()
	{
		return Queue;
	}

	FVulkanCommandBufferPool *AcquireCommandBufferPool();

  private:
	uint32_t FamilyIndex{};
	uint32_t QueueIndex{};
	EVulkanQueueType QueueType;
	FVulkanDevice &Device;
	vk::Queue Queue{VK_NULL_HANDLE};
	// only support primary cmd buffer pool
	std::unique_ptr<FVulkanCommandBufferPool> CommandBufferPool;
};
