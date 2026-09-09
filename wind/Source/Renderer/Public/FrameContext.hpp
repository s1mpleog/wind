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

	FVulkanSemaphore *GetImageAvailableSemaphore();
	FVulkanSemaphore *GetRenderFinishedSemaphore();
	FVulkanFence *GetInFlightFence();
	inline FVulkanCommandBuffer *GetCommandBuffer()
	{
		return CmdBuffer;
	}
	inline FVulkanFence *GetPresentFence()
	{
		return PresentFence;
	}

	~FFrameContext();

  private:
	FVulkanFence *InFlightFence = nullptr;
	FVulkanFence *PresentFence = nullptr;
	FVulkanSemaphore *ImageAvailableSemaphore = nullptr;
	FVulkanSemaphore *RenderFinishedSemaphore = nullptr;
	FVulkanCommandBuffer *CmdBuffer = nullptr;
};
