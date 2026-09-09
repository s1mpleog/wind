#pragma once

#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"

#include <cstddef>
#include <cstdint>

class FVulkanDevice;
class FVulkanQueue;
class FVulkanCommandBufferPool;
class FVulkanContext;

enum class EVulkanCommandBufferType : uint8_t
{
	Primary,

	Count
};

class FVulkanCommandBuffer
{
  public:
	FVulkanCommandBuffer(FVulkanDevice &InDevice, FVulkanCommandBufferPool &InCommandBufferPool);

	void Reset();
	void Begin(vk::CommandBufferBeginInfo &InBeginInfo);
	void End();

	vk::CommandBuffer GetHandle() const
	{
		return Handle;
	}

	void BeginDynamicRendering(const vk::RenderingInfo &RenderingInfo);
	void EndDynamicRendering();

	void BindPipeline(vk::PipelineBindPoint BindPoint, vk::PipelineLayout PipelineLayout);

  private:
	void AllocMemory();
	FVulkanDevice &Device;
	FVulkanCommandBufferPool &CommandBufferPool;
	vk::CommandBuffer Handle = VK_NULL_HANDLE;

	friend class FVulkanCommandBufferPool;
};

class FVulkanCommandBufferPool
{
  public:
	FVulkanCommandBufferPool(FVulkanDevice &InDevice, FVulkanQueue &InQueue);
	~FVulkanCommandBufferPool();

	FVulkanQueue &GetQueue()
	{
		return Queue;
	}

	vk::CommandPool GetHandle() const
	{
		return Handle;
	}

  private:
	FVulkanDevice &Device;
	FVulkanQueue &Queue;
	vk::CommandPool Handle = VK_NULL_HANDLE;
	std::vector<FVulkanCommandBuffer *> CmdBuffers;

	FVulkanCommandBuffer *Create();

	// todo: better way to do it ?
	friend class FVulkanContext;
};
