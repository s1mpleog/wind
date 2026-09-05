#pragma once

// allocates cmd buffer
// provides api for end, begin, reset, handle, destroy
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"

class FVulkanDevice;
class FVulkanQueue;

class FVulkanCommandBuffer
{
  public:
	void Begin();
	void End();

	vk::CommandBuffer GetHandle() const
	{
		return Handle;
	}

	void BeginDynamicRendering(const vk::RenderingInfo &RenderingInfo);
	void EndDynamicRendering();

	void BindPipeline(vk::PipelineBindPoint BindPoint, vk::PipelineLayout PipelineLayout);

  private:
	vk::CommandBuffer Handle = VK_NULL_HANDLE;
};

class FVulkanCommandBufferPool
{
  public:
	FVulkanCommandBufferPool(FVulkanDevice &InDevice, FVulkanQueue InQueue);
};
