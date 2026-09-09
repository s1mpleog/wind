#include "Renderer.hpp"

#include "VulkanContext.hpp"
#include "VulkanDevice.h"
#include "VulkanSwapchain.hpp"
#include "VulkanSynchronization.hpp"

#include <spdlog/spdlog.h>

FVulkanRenderer::FVulkanRenderer(FVulkanContext *InContext) : Context(InContext)
{
}

void FVulkanRenderer::Initialize()
{
	if (SwapChain == nullptr)
	{
		// SwapChain;
	}

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
	FFrameContext &Frame = Frames[CurrentFrame];

	// wait for the previous frame to finish
	Frame.GetInFlightFence()->Wait();
	Frame.GetPresentFence()->Wait();

	uint32_t NextImageIndex = 0;
	VkResult Result =
	    vkAcquireNextImageKHR(Context->GetDevice()->GetHandle(), Context->GetSwapChain()->GetHandle(), UINT64_MAX,
	                          Frame.GetImageAvailableSemaphore()->GetHandle(), nullptr, &NextImageIndex);

	if (Result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		// recreate swapchain
		spdlog::info("invalid swapchain recreate");
	}

	if (Result == VK_SUCCESS)
	{
		spdlog::info("Got new swapchain image index: {}", NextImageIndex);
	}

	SwapChainImageIndex = NextImageIndex;

	Frame.GetInFlightFence()->Reset();
	Frame.GetPresentFence()->Reset();

	// spec says that cmd buffer in any other state than pending can be transitioned  to initial state
	// by calling VkResetCommandBuffer once the GPU process the previous frame it will transitioned the
	// state to executable since we are not using one time submit flag
	Frame.GetCommandBuffer()->Reset();

	vk::CommandBufferBeginInfo BeginInfo{};

	// the spec says Command Buffer from Initial state can be transitioned  to Recording state by calling
	// VkBeginCommandBuffer from here on we can also use VkCmd* calls to record
	Frame.GetCommandBuffer()->Begin(BeginInfo);

	// transition the swapchain from initial to color attachment optimal so gpu can render into
	vk::ImageMemoryBarrier2 SwapChainImageBarrier{};

	// specify the target image
	SwapChainImageBarrier.image = Context->GetSwapChainImage(SwapChainImageIndex);

	// the inital layout of allocated image is undefined
	SwapChainImageBarrier.oldLayout = vk::ImageLayout::eUndefined;
	// we want new layout to be suitable for color attachment it may or may not change the
	// underlying data and metadata in a way that is suitable for color attachment
	SwapChainImageBarrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;

	// producer side since our initial layout was undefined we can ignore producer side
	// in simple terms UNDEFINED said to driver i don't care about previous data you can discard it
	// we use None for that
	SwapChainImageBarrier.srcStageMask = vk::PipelineStageFlagBits2::eNone;
	SwapChainImageBarrier.srcAccessMask = vk::AccessFlagBits2::eNone;

	// consumer side who is consumer its color attachment when stage where GPU writes pixels to image
	// and GPU will write to it
	SwapChainImageBarrier.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
	SwapChainImageBarrier.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;

	// todo: learn about this
	SwapChainImageBarrier.subresourceRange = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

	vk::DependencyInfo DepInfo{};
	DepInfo.imageMemoryBarrierCount = 1;
	DepInfo.pImageMemoryBarriers = &SwapChainImageBarrier;

	Frame.GetCommandBuffer()->GetHandle().pipelineBarrier2(DepInfo);

	//================Dynamic Rendering=============================
	std::array<float, 4> ClearColor{0.055F, 0.0577F, 0.055F, 1.0F};

	// todo: let FVulkanCommandBuffer handle this
	vk::RenderingAttachmentInfo RenderingAttachInfo{};
	RenderingAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	RenderingAttachInfo.imageView = Context->GetSwapChainImageView(SwapChainImageIndex);
	// clear during load
	RenderingAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
	// store it (learn more about it)
	RenderingAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;
	RenderingAttachInfo.clearValue.color = ClearColor;

	// do Depth Attacment here

	// structure specifying render pass instance begin info
	vk::RenderingInfo RenderingInfo{};
	RenderingInfo.colorAttachmentCount = 1;
	RenderingInfo.pColorAttachments = &RenderingAttachInfo;

	vk::Extent2D SwapChainExtent = Context->GetSwapChain()->GetExtent();

	// todo: extent from swapchain
	vk::Rect2D RenderArea = {0, SwapChainExtent};

	RenderingInfo.renderArea = RenderArea;
	RenderingInfo.layerCount = 1;

	// begin a dynamic rendering instance now the command buffer is ready to record draw commands
	Frame.GetCommandBuffer()->GetHandle().beginRendering(RenderingInfo);
}

void FVulkanRenderer::Draw()
{
	// submit draw calls here whatever i wanna draw

	FFrameContext &Frame = Frames[CurrentFrame];

	vk::Rect2D Scissor{0};
	Scissor.extent = Context->GetSwapChain()->GetExtent();

	vk::Viewport ViewPort{};
	ViewPort.x = 0.0F;
	ViewPort.y = 0.0F;
	ViewPort.width = static_cast<float>(Context->GetSwapChain()->GetExtent().width);
	ViewPort.height = static_cast<float>(Context->GetSwapChain()->GetExtent().height);
	ViewPort.minDepth = 0.0F;
	ViewPort.maxDepth = 1.0F;

	Frame.GetCommandBuffer()->GetHandle().setViewport(0, ViewPort);
	Frame.GetCommandBuffer()->GetHandle().setScissor(0, Scissor);
}

// submission
void FVulkanRenderer::EndFrame()
{
	// end dynamic rendering instance
	// end the cmd buffer
	// transition image from color attach to present_khr
	// sumbit
	// present

	FFrameContext &Frame = Frames[CurrentFrame];

	// end the dynamic rendering instance
	Frame.GetCommandBuffer()->GetHandle().endRendering();

	// transition swapchain image from color attachment to present_khr
	vk::ImageMemoryBarrier2 SwapChainImageBarrier{};

	SwapChainImageBarrier.image = Context->GetSwapChainImage(SwapChainImageIndex);

	SwapChainImageBarrier.subresourceRange = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

	SwapChainImageBarrier.oldLayout = vk::ImageLayout::eAttachmentOptimal;
	// new layout should be optimal for presentation
	SwapChainImageBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;

	// =======producer side====================

	// producer point its color attachment output when GPU processed image
	SwapChainImageBarrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
	// producer operation it's write
	SwapChainImageBarrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;

	// ==========consumer side================

	// consumer point (syncrhonizes with producer side)
	// why None ? because presentation engine is not a part of vulkan its OS stuffs
	// presentation engine is not a graphics pipeline layout vulkan have no control
	// consumer point
	SwapChainImageBarrier.dstStageMask = vk::PipelineStageFlagBits2::eNone;
	// consumer operation
	SwapChainImageBarrier.dstAccessMask = vk::AccessFlagBits2::eNone;

	vk::DependencyInfo DepInfo{};
	DepInfo.imageMemoryBarrierCount = 1;
	DepInfo.pImageMemoryBarriers = &SwapChainImageBarrier;

	Frame.GetCommandBuffer()->GetHandle().pipelineBarrier2(DepInfo);

	// ===============End command buffer no more recording===============
	// now command buffer is in executable stage we can submit it into queue
	Frame.GetCommandBuffer()->End();

	//===================Submit to Queue========================
	vk::CommandBufferSubmitInfo CmdBufferSubmitInfo{};
	// specify the command buffer conceptually cmd buffer have the commands we recorded in Draw()
	CmdBufferSubmitInfo.commandBuffer = Frame.GetCommandBuffer()->GetHandle();

	// ===================Graphics queue should wait for image available from swapchain==========
	vk::SemaphoreSubmitInfo WaitInfo{};
	WaitInfo.semaphore = Frame.GetImageAvailableSemaphore()->GetHandle();
	// do all previous operations but wait at eColorAttachmentOutput until ImageAvailableSemaphore is signaled
	// during acquire call we passed ImageAvailableSemaphore in parameter so when that function returns
	// it will set the ImageAvailableSemaphore state to SIGNALED
	WaitInfo.stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;

	//=============Graphics queue should signal to presentation once it finishes rendering==================
	// it does not wait just signals
	vk::SemaphoreSubmitInfo SignalInfo{};
	SignalInfo.semaphore = Frame.GetRenderFinishedSemaphore()->GetHandle();
	// once the graphics pipeline completed
	SignalInfo.stageMask = vk::PipelineStageFlagBits2::eAllGraphics;

	vk::SubmitInfo2 SubmitInfo{};
	SubmitInfo.commandBufferInfoCount = 1;
	SubmitInfo.pCommandBufferInfos = &CmdBufferSubmitInfo;

	// wait info
	SubmitInfo.waitSemaphoreInfoCount = 1;
	SubmitInfo.pWaitSemaphoreInfos = &WaitInfo;

	// signal info
	SubmitInfo.signalSemaphoreInfoCount = 1;
	SubmitInfo.pSignalSemaphoreInfos = &SignalInfo;

	// abstract this into context there is too much getters :(
	// this is async call
	auto SubmitResult = Context->GetDevice()->GetGraphicsQueue()->GetHandle().submit2(
	    SubmitInfo, Frame.GetInFlightFence()->GetHandle());

	// TODO: error or skip frame umm i will go with skip frame does not make sense to crash engine
	// because of one frame failed
	CHECK(SubmitResult.has_value(), "Failed to submit");

	//=========================Prepare for presentation=========================

	const vk::SwapchainKHR SwapChain = Context->GetSwapChain()->GetHandle();
	const vk::Semaphore RenderFinishedSemaphore = Frame.GetRenderFinishedSemaphore()->GetHandle();

	// ==================maintaince 1 extension==========================
	const vk::Fence PresentFence = Frame.GetPresentFence()->GetHandle();

	vk::SwapchainPresentFenceInfoKHR PresentFenceInfo{};
	PresentFenceInfo.swapchainCount = 1;
	PresentFenceInfo.pFences = &PresentFence;
	//=================================================================

	vk::PresentInfoKHR PresentInfo{};
	PresentInfo.pNext = &PresentFenceInfo;
	PresentInfo.swapchainCount = 1;
	PresentInfo.pSwapchains = &SwapChain;
	PresentInfo.waitSemaphoreCount = 1;
	// wait for RenderFinishedSemaphore to be signaled before presenting
	// we set RenderFinishedSemaphore in submit signal info so when GPU process all graphics commands
	// it will set its state to SIGNALED
	PresentInfo.pWaitSemaphores = &RenderFinishedSemaphore;
	// which swapchain image we wanna present
	PresentInfo.pImageIndices = &SwapChainImageIndex;

	// there is weird issues with presntation and vulkan hpp use c api for it
	VkPresentInfoKHR PresentInfoC = static_cast<VkPresentInfoKHR>(PresentInfo);

	VkResult PresentResult = vkQueuePresentKHR(Context->GetDevice()->GetPresentQueue()->GetHandle(), &PresentInfoC);

	if (PresentResult != VK_SUCCESS && PresentResult != VK_SUBOPTIMAL_KHR && PresentResult != VK_ERROR_OUT_OF_DATE_KHR)
	{
		spdlog::info("Failed to submit");
	}

	CurrentFrame = (CurrentFrame + 1) % MAX_FRAME_IN_FLIGHT;
}
