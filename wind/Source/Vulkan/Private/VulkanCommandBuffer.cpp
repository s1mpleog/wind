#include "VulkanCommandBuffer.hpp"

#include "VulkanCheck.hpp"
#include "VulkanDevice.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"

#include <algorithm>

FVulkanCommandBuffer::FVulkanCommandBuffer(FVulkanDevice &InDevice, FVulkanCommandBufferPool &InCommandBufferPool)
    : Device(InDevice), CommandBufferPool(InCommandBufferPool) {};

void FVulkanCommandBuffer::AllocMemory()
{
	vk::CommandBufferAllocateInfo CmdBufferAllocInfo{};
	CmdBufferAllocInfo.commandBufferCount = 1;
	// TODO: add support for secondary cmd buffer
	CmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	CmdBufferAllocInfo.commandPool = CommandBufferPool.GetHandle();

	VERIFYVULKANRESULT_UNWRAP(CommandBuffers, Device.GetHandle().allocateCommandBuffers(CmdBufferAllocInfo));

	Handle = std::move(CommandBuffers.front());
}

FVulkanCommandBufferPool::FVulkanCommandBufferPool(FVulkanDevice &InDevice, FVulkanQueue &InQueue)
    : Device(InDevice), Queue(InQueue)
{
	vk::CommandPoolCreateInfo CmdPoolCreateInfo{};
	CmdPoolCreateInfo.queueFamilyIndex = Queue.GetFamilyIndex();
	// the specs says: eResetCommandBuffer allows any command buffer allocated from the pool
	// to be individually reset to be initial state either by calling vk::ResetCommandBuffer or via implicit reset
	CmdPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

	VERIFYVULKANRESULT_UNWRAP(CommandPool, Device.GetHandle().createCommandPool(CmdPoolCreateInfo));

	Handle = std::move(CommandPool);
};

FVulkanCommandBufferPool::~FVulkanCommandBufferPool()
{
	if (Handle != VK_NULL_HANDLE)
	{
		Device.GetHandle().destroyCommandPool(Handle);
	}
}
