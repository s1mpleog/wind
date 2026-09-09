#pragma once

#include "FrameContext.hpp"

#include <vector>

class FVulkanContext;
class FVulkanSwapChain;

class FVulkanRenderer
{
  public:
	FVulkanRenderer(FVulkanContext *InContext);

	static constexpr uint32_t MAX_FRAME_IN_FLIGHT = 3;

	void Initialize();

	void BeginFrame();
	void Draw();
	void EndFrame();

  private:
	FVulkanContext *Context;
	std::unique_ptr<FVulkanSwapChain> SwapChain;
	std::uint32_t CurrentFrame = 0;
	std::uint32_t SwapChainImageIndex = 0;
	std::vector<FFrameContext> Frames;
};