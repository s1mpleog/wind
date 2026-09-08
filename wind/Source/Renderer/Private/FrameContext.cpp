#include "FrameContext.hpp"

#include "VulkanContext.hpp"
#include "VulkanSynchronization.hpp"

// todo: let context expose create fence and swapchain don't voilate the boundary
FFrameContext::FFrameContext(FVulkanContext *Context)
    : InFlightFence(new FVulkanFence(*Context->GetDevice())),
      ImageAvailableSemaphore(new FVulkanSemaphore(*Context->GetDevice())),
      RenderFinishedSemaphore(new FVulkanSemaphore(*Context->GetDevice())),
      CmdBuffer(Context->CreateGraphicsCommandBuffer())
{
	InFlightFence->Create(true);
}

FFrameContext::~FFrameContext()
{
	if (InFlightFence != nullptr)
	{
		InFlightFence->Destroy();
		delete InFlightFence;
	}

	if (ImageAvailableSemaphore != nullptr)
	{
		ImageAvailableSemaphore->Destroy();
		delete ImageAvailableSemaphore;
	}

	if (RenderFinishedSemaphore != nullptr)
	{
		RenderFinishedSemaphore->Destroy();
		delete RenderFinishedSemaphore;
	}
}
