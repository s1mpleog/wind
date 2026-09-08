#include "VulkanQueue.hpp"

#include "Check.hpp"
#include "VulkanCommandBuffer.hpp"
#include "VulkanDevice.h"

#include <cstdint>
#include <memory>

FVulkanQueue::FVulkanQueue(FVulkanDevice &InDevice, uint32_t InFamilyIndex, EVulkanQueueType InQueueType)
    : FamilyIndex(InFamilyIndex), QueueIndex(0), QueueType(InQueueType), Device(InDevice)
{
	Device.GetHandle().getQueue(FamilyIndex, QueueIndex, &Queue);
	WIND_LOG(info, "Created queue type: {} index: {}", GetVulkanQueueTypeName(QueueType), FamilyIndex);
};

// FVulkanQueue owns FVulkanCommandBufferPool and FVulkanCommandBufferPool owns N FVulkanCommandBuffer
// so each FVulkanQueue in FVulkanDevice have there own seperate cmd pool and cmd buffer
FVulkanCommandBufferPool *FVulkanQueue::AcquireCommandBufferPool()
{
	if (CommandBufferPool != nullptr)
	{
		return CommandBufferPool.get();
	}

	CommandBufferPool = std::make_unique<FVulkanCommandBufferPool>(Device, *this);
	return CommandBufferPool.get();
}
