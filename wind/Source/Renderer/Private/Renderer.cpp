#include "Renderer.hpp"

#include "VulkanContext.hpp"

#include <spdlog/spdlog.h>

FVulkanRenderer::FVulkanRenderer(FVulkanContext *InContext) : Context(InContext)
{
	Frames.reserve(MAX_FRAME_IN_FLIGHT);
	for (uint32_t Index = 0; Index < MAX_FRAME_IN_FLIGHT; ++Index)
	{
		// Renderer stores *Context;
		// *Context is valid till Engine is valid
		// FFrameContext does not owns Context it take pointer to it
		// in Engine the Renderer will be destroyed first then Context
		// so Renderer <-> Ownership is valid
		// inside each FFrameContext {fence, semaphore} stores ref to FVulkanDevice
		// also in FFrameContext i am asking for context to create a cmd buffer
		// which internally calls FVulkanQueue which own FVulkanCommandBufferPool which owns N FVulkanCommandBuffer
		// in ~FFrameContext() the FVulkanDevice devices needs to be valid because {fence, semaphore} owns it
		// the thing is Renderer will get destroyed which internally destroy N FFrameContext which calls
		// destructor to destroy fence and semaphore then Context will get destroyed
		Frames.emplace_back(Context);
	}
}

void FVulkanRenderer::BeginFrame()
{
	// auto CurrentFrame = FrameContext.GetCurrentFrameIndex();
	// FFrameContext &Frame = Frames[CurrentFrame];
}

void FVulkanRenderer::Draw()
{
}

void FVulkanRenderer::EndFrame()
{
	CurrentFrame = (CurrentFrame + 1) % MAX_FRAME_IN_FLIGHT;
}