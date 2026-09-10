#pragma once

#include "FrameContext.hpp"
#include "VulkanGenericPlatform.h"

#include <vector>

class FVulkanContext;
class FVulkanSwapChain;

struct FPresentationTarget
{
	FVulkanGenericPlatformWindowContext WindowContext;
	uint32_t Width;
	uint32_t Height;
};

class FVulkanRenderer
{
  public:
	static constexpr uint32_t MAX_FRAME_IN_FLIGHT = 3;

	FVulkanRenderer(FVulkanContext *InContext, FPresentationTarget &InPresentationTarget);
	~FVulkanRenderer();

	void Initialize();

	// temporary
	// use expected also
	FFrameResult BeginFrame(uint32_t InWidth, uint32_t InHeight);
	void Draw();
	FFrameResult EndFrame();

  private:
	FVulkanContext *Context;
	std::unique_ptr<FVulkanSwapChain> SwapChain;
	std::uint32_t CurrentFrame = 0;
	std::uint32_t SwapChainImageIndex = 0;
	std::vector<FFrameContext> Frames;
	FPresentationTarget PresentationTarget;
};