#pragma once

#include "FrameContext.hpp"

class FVulkanContext;

class FVulkanRenderer
{
  public:
	FVulkanRenderer(FVulkanContext *InContext);

	static constexpr uint32_t MAX_FRAME_IN_FLIGHT = 3;

	void BeginFrame();
	void Draw();
	void EndFrame();

  private:
	FVulkanContext *Context;
	FFrameContext FrameContext;
};