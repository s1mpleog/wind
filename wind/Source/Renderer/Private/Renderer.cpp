#include "Renderer.hpp"

#include "VulkanContext.hpp"

FVulkanRenderer::FVulkanRenderer(FVulkanContext *InContext) : Context(InContext), FrameContext(InContext)
{
}

void FVulkanRenderer::BeginFrame()
{
	auto CurrentFrame = FrameContext.GetCurrentFrameIndex();
}

void FVulkanRenderer::Draw()
{
}

void FVulkanRenderer::EndFrame()
{
}