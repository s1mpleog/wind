#pragma once

#include "Configuration.hpp"

#include <memory>

class FVulkanCore;
class FVulkanCommandBufferPool;
class FVulkanDevice;
class FVulkanQueue;
class FVulkanFence;
class FVulkanSwapChain;
class FVulkanGenericPlatformWindowContext;

class FVulkanContext
{
  public:
	FVulkanContext(FConfiguration InConfig);
	~FVulkanContext();

	void Initialize();

	// create the swapchain
	// should CreateSwapChain() will return *FVulkanSwapChain or void ?
	// should Recreate take FVulkanSwapChain* as parameter ?

	FVulkanSwapChain *CreateSwapchain(FVulkanGenericPlatformWindowContext &InWindowContext, uint32_t InWidth,
	                                  uint32_t InHeight, uint32_t *InDesiredImageCount);

	// FVulkanDevice *GetDevice() const;
	// vk::Instance GetInstance() const;
	// FVulkanQueue *GetGraphicsQueue() const;

  private:
	bool bHasInitialized = false;
	FConfiguration Config;
	std::unique_ptr<FVulkanCore> Core;
	std::unique_ptr<FVulkanSwapChain> SwapChain;
	std::unique_ptr<FVulkanCommandBufferPool> CommandBufferPool;
};
