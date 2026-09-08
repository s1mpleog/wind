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

	inline uint32_t GetCurrentFrameIndex() const
	{
		return CurrentFrame;
	}

  private:
	std::uint32_t CurrentFrame = 0;
	FVulkanFence *InFlightFence = nullptr;
	FVulkanSemaphore *ImageAvailableSemaphore = nullptr;
	FVulkanSemaphore *RenderFinishedSemaphore = nullptr;
	FVulkanCommandBuffer *CmdBuffer = nullptr;
};
