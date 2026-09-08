#pragma once

#include <cstdint>

class FVulkanFence;
class FVulkanSemaphore;
class FVulkanCommandBuffer;
class FVulkanContext;

class FFrameContext
{
  public:
	FFrameContext(FVulkanContext &Context)
	{
		// allocate objects through context
	}

  private:
	std::uint32_t CurrentFrame = 0;
	FVulkanFence *Fence;
	FVulkanSemaphore *Semaphore;
	FVulkanCommandBuffer *CmdBuffer;
};
