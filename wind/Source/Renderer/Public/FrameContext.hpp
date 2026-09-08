#pragma once

#include <cstdint>

class FVulkanFence;
class FVulkanSemaphore;
class FVulkanCommandBuffer;
class FVulkanContext;

class FFrameContext
{
  public:
	FFrameContext(FVulkanContext *Context);
	~FFrameContext();

  private:
	FVulkanFence *InFlightFence = nullptr;
	FVulkanSemaphore *ImageAvailableSemaphore = nullptr;
	FVulkanSemaphore *RenderFinishedSemaphore = nullptr;
	FVulkanCommandBuffer *CmdBuffer = nullptr;
};
