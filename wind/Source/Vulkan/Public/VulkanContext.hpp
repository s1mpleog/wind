#pragma once

#include "Configuration.hpp"

#include <memory>

class FVulkanCore;
class FVulkanCommandBufferPool;

class FVulkanContextTest
{
  public:
	FVulkanContextTest(FConfiguration InConfig);
	~FVulkanContextTest();

	void Initialize();

  private:
	bool bHasInitialized = false;
	FConfiguration Config;
	std::unique_ptr<FVulkanCore> Core;
	std::unique_ptr<FVulkanCommandBufferPool> CommandBufferPool;
};
