#include "VulkanContext.hpp"

#include "Check.hpp"
#include "Configuration.hpp"
#include "VulkanCommandBuffer.hpp"
#include "VulkanCore.hpp"
#include "VulkanDevice.h"
#include "VulkanSwapchain.hpp"

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

		// creates command pool
		CommandBufferPool =
		    std::make_unique<FVulkanCommandBufferPool>(*Core->GetDevice(), *Core->GetDevice()->GetGraphicsQueue());

		auto cmd = CommandBufferPool->Create();
	}
}

FVulkanSwapChain *FVulkanContext::CreateSwapchain(FVulkanGenericPlatformWindowContext &InWindowContext,
                                                  uint32_t InWidth, uint32_t InHeight, uint32_t *InDesiredImageCount)
{
	CHECK(bHasInitialized, "Vulkan Context is not initialized");

	if (SwapChain != nullptr)
	{
		return SwapChain.get();
	}

	SwapChain = std::make_unique<FVulkanSwapChain>(*Core.get());
	CHECK(SwapChain);

	SwapChain->Create(InWindowContext, InWidth, InHeight, InDesiredImageCount, nullptr);

	return SwapChain.get();
}

// FVulkanDevice *FVulkanContext::GetDevice() const
//{
//	return Core->GetDevice();
// }
//
// vk::Instance FVulkanContext::GetInstance() const
//{
//	return Core->GetInstance();
// }
//
// FVulkanQueue *FVulkanContext::GetGraphicsQueue() const
//{
//	return Core->GetDevice()->GetGraphicsQueue();
// }

FVulkanContext::~FVulkanContext()
{
	if (CommandBufferPool)
	{
		CommandBufferPool.reset();
	}

	if (SwapChain)
	{
		SwapChain->Destroy(nullptr);
		SwapChain.reset();
	}

	if (Core)
	{
		Core.reset();
	}
}
