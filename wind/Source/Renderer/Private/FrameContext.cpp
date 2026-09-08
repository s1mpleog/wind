#include "FrameContext.hpp"

#include "VulkanContext.hpp"
#include "VulkanSynchronization.hpp"

#include <spdlog/spdlog.h>

FFrameContext::FFrameContext(FVulkanContext *Context)
    : InFlightFence(new FVulkanFence(*Context->GetDevice())),
      ImageAvailableSemaphore(new FVulkanSemaphore(*Context->GetDevice())),
      RenderFinishedSemaphore(new FVulkanSemaphore(*Context->GetDevice())),
      CmdBuffer(Context->CreateGraphicsCommandBuffer())
{
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
