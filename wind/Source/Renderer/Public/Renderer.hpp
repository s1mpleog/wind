#pragma once

#include "FrameContext.hpp"

#include <vector>

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
	std::uint32_t CurrentFrame = 0;
	std::vector<FFrameContext> Frames;
};