#include "VulkanContext.hpp"

#include "Check.hpp"
#include "Configuration.hpp"
#include "VulkanCommandBuffer.hpp"
#include "VulkanCore.hpp"
#include "VulkanDevice.h"

#include <memory>

// provide a mechanism to create cmd buffers
// provide a mechanism to create/recreate swapchain
// provide a mechanism to create fence and semaphore
// provide a mechanism to get swapchain images

FVulkanContext::FVulkanContext(FConfiguration InConfig) : Config(std::move(InConfig)) {};

void FVulkanContext::Initialize()
{
	CHECK(!bHasInitialized, "Vulkan Context is already initialized");

	if (!bHasInitialized)
	{
		// creates instance and selects physical device
		Core = std::make_unique<FVulkanCore>(Config);
		// init logical device creation
		Core->Initialize();

		bHasInitialized = true;
	}
}

FVulkanDevice *FVulkanContext::GetDevice() const
{
	return Core->GetDevice();
}

FVulkanCommandBuffer *FVulkanContext::CreateGraphicsCommandBuffer()
{
	FVulkanCommandBufferPool *Pool = Core->GetDevice()->GetGraphicsQueue()->AcquireCommandBufferPool();
	return Pool->Create();
}

FVulkanContext::~FVulkanContext()
{
	if (Core)
	{
		Core.reset();
	}
}
