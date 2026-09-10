#include "Renderer.hpp"

#include "VulkanContext.hpp"
#include "VulkanDevice.h"
#include "VulkanGenericPlatform.h"
#include "VulkanSwapchain.hpp"
#include "VulkanSynchronization.hpp"

#include <spdlog/spdlog.h>

FVulkanRenderer::FVulkanRenderer(FVulkanContext *InContext, FPresentationTarget &InPresentationTarget)
    : Context(InContext), PresentationTarget(InPresentationTarget)
{
}

void FVulkanRenderer::Initialize()
{
	if (SwapChain == nullptr)
	{
		uint32_t DesiredImageCount = 3;
		SwapChain = std::make_unique<FVulkanSwapChain>(*Context->GetCore());
		SwapChain->Create(PresentationTarget.WindowContext, PresentationTarget.Width, PresentationTarget.Height,
		                  &DesiredImageCount, nullptr);
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

FFrameResult FVulkanRenderer::BeginFrame(uint32_t InWidth, uint32_t InHeight)
{
	FFrameContext &Frame = Frames[CurrentFrame];

	// wait for the previous frame to finish
	Frame.GetInFlightFence()->Wait();
	Frame.GetPresentFence()->Wait();

	uint32_t NextImageIndex = 0;
	VkResult Result = vkAcquireNextImageKHR(Context->GetDevice()->GetHandle(), SwapChain->GetHandle(), UINT64_MAX,
	                                        Frame.GetImageAvailableSemaphore()->GetHandle(), nullptr, &NextImageIndex);

	if (Result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		// recreate swapchain
		spdlog::info("invalid swapchain recreate");

		uint32_t DesiredImageCount = 3;

		FVulkanSwapchainRecreateInfo RecreateInfo = {.SwapChain = SwapChain->GetHandle(),
		                                             .Surface = SwapChain->GetSurface()};

		SwapChain->Create(PresentationTarget.WindowContext, InWidth, InHeight, &DesiredImageCount, &RecreateInfo);

		return std::unexpected(EFrameError::OutOfDate);
	}

	if (Result != VK_SUCCESS && Result != VK_SUBOPTIMAL_KHR && Result != VK_ERROR_OUT_OF_DATE_KHR)
	{
		return std::unexpected(EFrameError::Fatal);
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
	SwapChainImageBarrier.image = SwapChain->GetImage(SwapChainImageIndex);

	// the inital layout of allocated image is undefined
	SwapChainImageBarrier.oldLayout = vk::ImageLayout::eUndefined;
	// we want new layout to be suitable for color attachment it may or may not change the
	// underlying data and metadata in a way that is suitable for color attachment
	SwapChainImageBarrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;

	SwapChainImageBarrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
	// producer side since our initial layout was undefined we can ignore producer side operation
	// in simple terms UNDEFINED said to driver i don't care about previous data you can discard it
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

	//[NVIDIA] Clearing image with format VK_FORMAT_B8G8R8A8_SRGB without a 1.0f or 0.0f clear color. The clear will not
	//get compressed in the GPU, harming performance. This can be fixed using a clear color of VkClearColorValue{0.0f,
	//0.0f, 0.0f, 0.0f}, or VkClearColorValue{1.0f, 1.0f, 1.0f, 1.0f}.
	std::array<float, 4> ClearColor{1.0F, 1.0F, 1.0F, 1.0F};

	// todo: let FVulkanCommandBuffer handle this
	vk::RenderingAttachmentInfo RenderingAttachInfo{};
	RenderingAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	RenderingAttachInfo.imageView = SwapChain->GetImageView(SwapChainImageIndex);
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

	vk::Rect2D RenderArea = {0, SwapChain->GetExtent()};

	RenderingInfo.renderArea = RenderArea;
	RenderingInfo.layerCount = 1;

	// begin a dynamic rendering instance now the command buffer is ready to record draw commands
	Frame.GetCommandBuffer()->GetHandle().beginRendering(RenderingInfo);

	return {};
}

void FVulkanRenderer::Draw()
{
	// submit draw calls here whatever i wanna draw

	FFrameContext &Frame = Frames[CurrentFrame];

	vk::Rect2D Scissor{0};
	Scissor.extent = SwapChain->GetExtent();

	vk::Viewport ViewPort{};
	ViewPort.x = 0.0F;
	ViewPort.y = 0.0F;
	ViewPort.width = static_cast<float>(SwapChain->GetExtent().width);
	ViewPort.height = static_cast<float>(SwapChain->GetExtent().height);
	ViewPort.minDepth = 0.0F;
	ViewPort.maxDepth = 1.0F;

	Frame.GetCommandBuffer()->GetHandle().setViewport(0, ViewPort);
	Frame.GetCommandBuffer()->GetHandle().setScissor(0, Scissor);
}

// submission
FFrameResult FVulkanRenderer::EndFrame()
{
	FFrameContext &Frame = Frames[CurrentFrame];

	// end the dynamic rendering instance
	Frame.GetCommandBuffer()->GetHandle().endRendering();

	// transition swapchain image from color attachment to present_khr
	vk::ImageMemoryBarrier2 SwapChainImageBarrier{};

	SwapChainImageBarrier.image = SwapChain->GetImage(SwapChainImageIndex);

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
	SignalInfo.stageMask = vk::PipelineStageFlagBits2::eAllCommands;

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

	// useful so we can write device lost error in crash file
	if (!SubmitResult && SubmitResult.error() == vk::Result::eErrorDeviceLost)
	{
		return std::unexpected(EFrameError::DeviceLost);
	}

	if (!SubmitResult && SubmitResult.error() != vk::Result::eSuccess)
	{
		return std::unexpected(EFrameError::Fatal);
	}

	//=========================Prepare for presentation=========================

	const vk::SwapchainKHR SwapChainHandle = SwapChain->GetHandle();
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
	PresentInfo.pSwapchains = &SwapChainHandle;
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
		return std::unexpected(EFrameError::Fatal);
	}

	CurrentFrame = (CurrentFrame + 1) % MAX_FRAME_IN_FLIGHT;
}

FVulkanRenderer::~FVulkanRenderer()
{
	Context->GetDevice()->WaitUntilIdle();

	if (SwapChain != nullptr)
	{
		SwapChain->Destroy(nullptr);
		SwapChain.reset();
	}
}
