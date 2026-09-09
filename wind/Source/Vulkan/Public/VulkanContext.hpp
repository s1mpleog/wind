#pragma once

#include "Configuration.hpp"

#include <memory>
#include <vulkan/vulkan.hpp>

class FVulkanCore;
class FVulkanCommandBufferPool;
class FVulkanDevice;
class FVulkanQueue;
class FVulkanFence;
class FVulkanSwapChain;
class FVulkanGenericPlatformWindowContext;
class FVulkanCommandBuffer;

class FVulkanContext
{
  public:
	FVulkanContext(FConfiguration InConfig);
	~FVulkanContext();

	void Initialize();

	FVulkanSwapChain *CreateSwapchain(FVulkanGenericPlatformWindowContext &InWindowContext, uint32_t InWidth,
	                                  uint32_t InHeight, uint32_t *InDesiredImageCount);
	FVulkanSwapChain *GetSwapChain();
	vk::Image GetSwapChainImage(uint32_t Index) const;
	vk::ImageView GetSwapChainImageView(uint32_t Index) const;

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
	std::unique_ptr<FVulkanSwapChain> SwapChain;
	std::unique_ptr<FVulkanCommandBufferPool> CommandBufferPool;
};
