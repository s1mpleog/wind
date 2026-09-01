#include "VulkanQueue.hpp"

#include "Check.hpp"
#include "VulkanDevice.h"

FVulkanQueue::FVulkanQueue(FVulkanDevice &InDevice, uint32 InFamilyIndex, EVulkanQueueType InQueueType)
    : FamilyIndex(InFamilyIndex), QueueIndex(0), QueueType(InQueueType), Device(InDevice)
{
	Device.GetHandle().getQueue(FamilyIndex, QueueIndex, &Queue);
	WIND_LOG(info, "Created queue type: {} index: {}", GetVulkanQueueTypeName(QueueType), FamilyIndex);
};
