#include "VulkanCommandBuffer.hpp"

#include "VulkanCheck.hpp"
#include "VulkanDevice.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"

#include <algorithm>
#include <ranges>

FVulkanCommandBuffer::FVulkanCommandBuffer(FVulkanDevice &InDevice, FVulkanCommandBufferPool &InCommandBufferPool)
    : Device(InDevice), CommandBufferPool(InCommandBufferPool)
{
	AllocMemory();
};

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

// todo: error handling
void FVulkanCommandBuffer::Reset()
{
	Handle.reset();
}

void FVulkanCommandBuffer::Begin(vk::CommandBufferBeginInfo &InBeginInfo)
{
	Handle.begin(InBeginInfo);
}

void FVulkanCommandBuffer::End()
{
	Handle.end();
}

FVulkanCommandBufferPool::FVulkanCommandBufferPool(FVulkanDevice &InDevice, FVulkanQueue &InQueue)
    : Device(InDevice), Queue(InQueue)
{
	vk::CommandPoolCreateInfo CmdPoolCreateInfo{};
	CmdPoolCreateInfo.queueFamilyIndex = Queue.GetFamilyIndex();
	// the spec says: eResetCommandBuffer allows any command buffer allocated from the pool
	// to be individually reset to be initial state either by calling vk::ResetCommandBuffer or via implicit reset
	CmdPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

	VERIFYVULKANRESULT_UNWRAP(CommandPool, Device.GetHandle().createCommandPool(CmdPoolCreateInfo));

	Handle = std::move(CommandPool);
};

FVulkanCommandBufferPool::~FVulkanCommandBufferPool()
{
	for (auto *CmdBuffer : CmdBuffers)
	{
		delete CmdBuffer;
	}

	if (Handle != VK_NULL_HANDLE)
	{
		Device.GetHandle().destroyCommandPool(Handle);
	}
}

FVulkanCommandBuffer *FVulkanCommandBufferPool::Create()
{
	// if there is free cmd buffers then pop it and return
	// else allocate a cmd buffer add to CmdBuffers and return
	// free_cmd:    [cb_1, cb_2, cb_3, ...., cb_n]
	// cmd_buffers: []

	FVulkanCommandBuffer *CmdBuffer = new FVulkanCommandBuffer(Device, *this);
	CHECK(CmdBuffer);

	CmdBuffers.push_back(CmdBuffer);

	return CmdBuffer;
}
