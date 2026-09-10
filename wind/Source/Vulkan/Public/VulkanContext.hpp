#pragma once

#include "Configuration.hpp"

#include <memory>
#include <vulkan/vulkan.hpp>

class FVulkanCore;
class FVulkanCommandBufferPool;
class FVulkanDevice;
class FVulkanQueue;
class FVulkanFence;
class FVulkanCommandBuffer;

class FVulkanContext
{
  public:
	FVulkanContext(FConfiguration InConfig);
	~FVulkanContext();

	void Initialize();

	FVulkanCore *GetCore() const
	{
		return Core.get();
	}

	FVulkanDevice *GetDevice() const;

	FVulkanCommandBuffer *CreateGraphicsCommandBuffer();

  private:
	bool bHasInitialized = false;
	FConfiguration Config;
	std::unique_ptr<FVulkanCore> Core;
	std::unique_ptr<FVulkanCommandBufferPool> CommandBufferPool;
};
