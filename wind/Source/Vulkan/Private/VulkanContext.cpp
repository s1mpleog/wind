#include "VulkanContext.hpp"

#include "Check.hpp"
#include "Configuration.hpp"
#include "VulkanCommandBuffer.hpp"
#include "VulkanCore.hpp"
#include "VulkanDevice.h"
#include "VulkanSwapchain.hpp"

#include <memory>

FVulkanContextTest::FVulkanContextTest(FConfiguration InConfig) : Config(std::move(InConfig)) {};

void FVulkanContextTest::Initialize()
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
	}
}

FVulkanContextTest::~FVulkanContextTest()
{
	if (CommandBufferPool)
	{
		CommandBufferPool.reset();
	}

	if (Core)
	{
		Core.reset();
	}
}
