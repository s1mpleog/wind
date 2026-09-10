#include "Vulkan/Renderer.hpp"

#include "Config.hpp"
#include "Error.hpp"
#include "Platform/Window.hpp"
#include "Resources/ResourceManager.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Configuration.hpp"
#include "Vulkan/Core/Context.hpp"
#include "Vulkan/Core/Swapchain.hpp"
#include "Vulkan/Core/Synchroization.hpp"
#include "Vulkan/Frame/FrameContext.hpp"
#include "Vulkan/Graphics/PipelineManager.hpp"
#include "Vulkan/Graphics/ShaderTypes.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/trigonometric.hpp"
#include "spdlog/spdlog.h"

#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_to_string.hpp>

WIND_NODISCARD auto FRenderer::Create(FConfiguration Cfg, const FUWindow &Window, const FVulkanContext *Context,
                                      FUResourceManager *ResourceManager,
                                      FUPipelineManager *PipelineManager) WIND_NOEXCEPT -> TWindResult<FRenderer>
{
	// auto context = std::make_unique<VulkanContext>(WIND_TRY(create_context(window, cfg)));
	auto [width, heigth] = Window.DrawableSize();

	auto SwapchainContext = WIND_TRY(CreateSwapchain(Cfg, width, heigth, Context->Surface, Context->GpuDevice));

	// create does not stores reference of device or graphics pool
	auto FrameContext =
	    WIND_TRY(CreateFrame(MaxFrameInFlight, Context->GpuDevice.Device, Context->GpuDevice.GraphicsPool));

	// create depth image
	WIND_TRY(ResourceManager->CreateDefaultDepthImage(SwapchainContext.Extent.width, SwapchainContext.Extent.height));

	// auto ubo_handle = WIND_TRY(resource_manager->create_dynamic_uniform_buffer(sizeof(UboInstance)));

	return FRenderer(Cfg, Context, std::move(SwapchainContext), std::move(FrameContext), ResourceManager,
	                 PipelineManager, TDynamicBufferHandle{.Index = 0});
}

WIND_NODISCARD auto FRenderer::Begin(uint32 Width, uint32 Height) WIND_NOEXCEPT -> TWindResult<void>
{
	// get a frame
	auto *Frame = &FrameContext[CurrentFrame];

	// wait for fences (previous frame to complete)
	// vk::WaitForFences will return once fence is in signaled state so like once GPU processed the previous
	// frame it will set the fence to signaled and this will return
	WIND_TRY(Frame->WaitInFlightFence(Context->GpuDevice.Device));
	WIND_TRY(Frame->WaitPresentFence(Context->GpuDevice.Device));

	uint32_t ImageIndex{0};

	// we are using c-api here because c++ library is acting weird here its returning error
	// ask presentation engine for next swapchain image handle once this function return then image_available semaphore
	// will be SIGNALED
	VkResult RawResult = Context->GpuDevice.Device.getDispatcher()->vkAcquireNextImageKHR(
	    *Context->GpuDevice.Device, *SwapchainContext.Handle, UINT64_MAX, *Frame->ImageAvailable, VK_NULL_HANDLE,
	    &ImageIndex);

	auto SwapchainResult = static_cast<vk::Result>(RawResult);

	if (SwapchainResult == vk::Result::eErrorOutOfDateKHR)
	{
		// no image was acquired recreate swapchain acquire again
		// stop the current frame

		// TODO: change this with different option
		WIND_TRY(Context->GpuDevice.Device.waitIdle());

		spdlog::info("out of date swapchain recreating");

		auto OldSwapchain = std::move(SwapchainContext);
		auto NewSwapchain = WIND_TRY(
		    CreateSwapchain(Config, Width, Height, Context->Surface, Context->GpuDevice, &OldSwapchain.Handle));

		SwapchainContext = std::move(NewSwapchain);

		// recreate the depth buffer
		WIND_TRY(ResourceManager->CreateDefaultDepthImage(Width, Height));

		// this does not means error i am doing this so in draw call i check this error code
		// if error == out_of_date then continue otherwise return error
		WIND_ERR(WindError::vulkan(ErrorCode::SwapchainOutOfDate, SwapchainResult));
	}

	if (SwapchainResult != vk::Result::eSuccess && SwapchainResult != vk::Result::eSuboptimalKHR &&
	    SwapchainResult != vk::Result::eErrorOutOfDateKHR)
	{
		WIND_ERR(WindError::vulkan(ErrorCode::SwapchainSuboptimal, SwapchainResult));
	}

	// vk::ResetFence the specs says when vk::ResetFence is executed on host (CPU), it defines
	// a fence un-signaled operation for each fence, which resets the fence to un-signaled state
	// we need this because vulkan states that we must not submit to queue with signaled fence
	// so we have to do it like if we submit to signaled fence then when cpu waits it will return
	// automatically since it is in signaled fence let GPU change from un-signaled to signaled when it
	// finishes the operation
	WIND_TRY(Frame->ResetInFlightFence(Context->GpuDevice.Device));
	WIND_TRY(Frame->ResetPresentFence(Context->GpuDevice.Device));

	// set the current image index to what swapchain index gave us
	CurrentImage = ImageIndex;

	// reset old command buffer
	// the specs says any state other than pending can be transitioned to initial state by
	// called vk::ResetCommandBuffer pending = when we do queue submit the specs also says application
	// must not modify command buffer which is in pending state
	WIND_TRY(Frame->ResetCmdBuffer());

	// begin recording command
	// set the command buffer state from initial to recording state by calling vk::BeginCommandBuffer
	// the specs says recording state can be used to record commands via vk::Cmd*
	WIND_TRY(Frame->Begin());

	// transition swapchain image from undefined to optimal for color attachment
	// we can't use undefined layout for color attachment aka rendering we must transition first
	TransitionImage(Frame->GraphicsCommandBuffer, SwapchainContext.Images[CurrentImage], vk::ImageLayout::eUndefined,
	                vk::ImageLayout::eColorAttachmentOptimal);

	std::array<float, 4> ClearColor{0.055F, 0.055F, 0.055F, 1.0F};

	vk::ClearColorValue Color{};
	Color.setFloat32(ClearColor);

	vk::RenderingAttachmentInfo ColorAttachInfo{};
	ColorAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	ColorAttachInfo.imageView = SwapchainContext.ImageViews[CurrentImage];
	ColorAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
	ColorAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;
	ColorAttachInfo.clearValue.color = Color;

	// create depth attachment
	vk::RenderingAttachmentInfo DepthAttachInfo{};
	DepthAttachInfo.imageView = ResourceManager->GetDefaultDepthImageView();
	DepthAttachInfo.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
	DepthAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
	DepthAttachInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
	DepthAttachInfo.clearValue.depthStencil.setDepth(1.0F);

	vk::Rect2D RenderArea{0, SwapchainContext.Extent};

	// information for dynamic rendering
	vk::RenderingInfo RenderingInfo{};
	RenderingInfo.colorAttachmentCount = 1;
	RenderingInfo.pColorAttachments = &ColorAttachInfo;
	RenderingInfo.pDepthAttachment = &DepthAttachInfo;
	RenderingInfo.renderArea = RenderArea;
	RenderingInfo.layerCount = 1;

	// begin a dynamic render pass instance the specs says after beginning a render pass instance
	// the command buffer is ready to record draw commands
	Frame->GraphicsCommandBuffer.beginRendering(RenderingInfo);

	return {};
}

auto FRenderer::DrawBuffer(FRenderObject Object, FRenderView CameraView,
                           vk::raii::CommandBuffer &CmdBuffer) WIND_NOEXCEPT -> void
{
	WIND_ASSERT(!Object.IsModelType && "Trying to draw buffer but object is model");

	const auto *Pipeline = PipelineManager->GetUnchecked(Object.PipelineHandle);
	CmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, Pipeline->GraphicsPipeline);

	const auto *VertexBuffer = ResourceManager->GetBufferUnchecked(Object.BufferAsset.VertexHandle);
	const auto *IndexBuffer = ResourceManager->GetBufferUnchecked(Object.BufferAsset.IndexHandle);

	auto PushConstant = FPushConstants{.Transform = CameraView.Projection * CameraView.View *
	                                                glm::translate(glm::mat4{1.0F}, glm::vec3{0.0F, 0.0F, -10.0F}),
	                                   .AlbedoTexture = 0,
	                                   .NormalIndex = 0,
	                                   .MetallicRoughnessIndex = 0,
	                                   .BaseColor = glm::vec4{0.0F, 0.0F, 0.0F, 0.0F}};

	CmdBuffer.pushConstants(Pipeline->PipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(FPushConstants),
	                        &PushConstant);

	std::array<vk::Buffer, 1> Buffers{VertexBuffer->Buffer};
	std::array<vk::DeviceSize, 1> Offsets{0};

	CmdBuffer.bindVertexBuffers(0, Buffers, Offsets);
	CmdBuffer.bindIndexBuffer(IndexBuffer->Buffer, 0, vk::IndexType::eUint16);

	CmdBuffer.drawIndexed(Object.BufferAsset.IndexCount, 1, 0, 0, 0);
}

auto FRenderer::DrawModel(FRenderObject Object, FRenderView CameraView,
                          vk::raii::CommandBuffer &CmdBuffer) WIND_NOEXCEPT -> void
{
	WIND_ASSERT(Object.IsModelType && "Trying to draw model but object is not model");

	const auto *Pipeline = PipelineManager->GetUnchecked(Object.PipelineHandle);

	CmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, Pipeline->GraphicsPipeline);

	const auto *Model = ResourceManager->GetModelUnchecked(Object.ModelHandle);
	const auto &Mesh = Model->Mesh;

	const auto &DescriptorSet = *ResourceManager->GetBindlessDescriptorSet();

	// auto  ubo_instance = UboInstance{.transform = camera_view.projection * camera_view.view * glm::mat4{1.0F}};
	// auto* mapped       = m_resource_manager->get_mapped_data_unchecked(m_frame_ubo);
	// std::memcpy(mapped, &ubo_instance, sizeof(ubo_instance));

	CmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, Pipeline->PipelineLayout, 0, *DescriptorSet, {0});

	std::array<vk::Buffer, 4> Buffers{Mesh.VertexBuffer.Buffer, Mesh.Normals.Buffer, Mesh.Uvs.Buffer,
	                                  Mesh.Tangents.Buffer};
	std::array<vk::DeviceSize, 4> Offsets{0, 0, 0, 0};

	CmdBuffer.bindVertexBuffers(0, Buffers, Offsets);
	CmdBuffer.bindIndexBuffer(Mesh.IndexBuffer.Buffer, 0, vk::IndexType::eUint32);

	for (const auto &Submesh : Mesh.SubMeshes)
	{
		const auto &Material = Model->Materials[Submesh.MaterialIndex];
		auto Transform = CameraView.Projection * CameraView.View *
		                 glm::rotate(glm::mat4{1.0F}, glm::radians(45.0F), glm::vec3{0.0F, 1.0F, 0.0F});

		FPushConstants Pc{
		    .Transform = Transform,
		    .AlbedoTexture = Material.AlbedoTexture ? Material.AlbedoTexture.value() : UINT32_MAX,
		    .NormalIndex = Material.NormalTexture ? Material.NormalTexture.value() : UINT32_MAX,
		    .MetallicRoughnessIndex =
		        Material.MetallicRoughnessTexture ? Material.MetallicRoughnessTexture.value() : UINT32_MAX,
		    .BaseColor = Material.BaseColor,
		};

		CmdBuffer.pushConstants(Pipeline->PipelineLayout,
		                        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0,
		                        sizeof(FPushConstants), &Pc);

		CmdBuffer.drawIndexed(Submesh.IndexCount, 1, Submesh.IndexOffset, 0, 0);
	}
}

auto FRenderer::SetupViewport(vk::raii::CommandBuffer &CmdBuffer) const WIND_NOEXCEPT -> void
{
	vk::Rect2D Scissor{0};
	Scissor.extent = SwapchainContext.Extent;

	vk::Viewport Viewport{};
	Viewport.x = 0.0F;
	Viewport.y = 0.0F;
	Viewport.width = static_cast<float>(SwapchainContext.Extent.width);
	Viewport.height = static_cast<float>(SwapchainContext.Extent.height);
	Viewport.minDepth = 0.0F;
	Viewport.maxDepth = 1.0F;

	CmdBuffer.setViewport(0, Viewport);
	CmdBuffer.setScissor(0, Scissor);
}

auto FRenderer::Draw(FRenderObject Object, FRenderView CameraView) WIND_NOEXCEPT -> void
{
	auto *Frame = &FrameContext[CurrentFrame];

	SetupViewport(Frame->GraphicsCommandBuffer);

	if (Object.IsModelType)
	{
		DrawModel(Object, CameraView, Frame->GraphicsCommandBuffer);
	}
	else
	{
		DrawBuffer(Object, CameraView, Frame->GraphicsCommandBuffer);
	}
}

auto FRenderer::End() WIND_NOEXCEPT -> void
{
	auto *Frame = &FrameContext[CurrentFrame];

	// end a dynamic render pass instance we already did required draw calls in draw() function
	// in begin() we did begin_rendering which says that we can record draw calls into cmd buffer after that
	// we did it in draw() now we have to end the instance no more draw cmd records
	Frame->GraphicsCommandBuffer.endRendering();

	// transition image from color_attachment_optimal to present_src_khr so presentation engine can
	// display it to monitor previous layout was color_attachment which was suitable for rendering now
	// we want a layout which will be suitable for presentation engine
	TransitionImage(Frame->GraphicsCommandBuffer, SwapchainContext.Images[CurrentImage],
	                vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR);

	// vk::EndCmdBuffer
	// when we did begin the state of command buffer was in recording now after end the state will transition to
	// executable state the specs says: The command buffer must have been in recording state, and if successful, is
	// moved to executable state so we can't call vk::EndCmdBuffer if we first did not called vk::CmdBegin a command
	// buffer in executable state can be submitted for execution, reset, freed or record another command buffer
	if (!Frame->End())
		return;

	vk::CommandBufferSubmitInfo CmdBufferSubmitInfo{};
	CmdBufferSubmitInfo.commandBuffer = Frame->GraphicsCommandBuffer;

	// this is wait semaphore remember in acquire_image call we set this image_available semaphore
	// so once acquire calls return the semaphore state will transition to signaled
	// stageMask = eColorAttachmentOutput so the idea is we can do previous pipeline operations
	// like vertex shader, primitive assembly, rasterization do those but before doing final
	// color attachment wait for swapchain image the image_available semaphore must needs to be signaled
	// before processing colorAttachmentOutput stage but wait for presentation engine to give image to render into
	vk::SemaphoreSubmitInfo WaitSemaphoreInfo{};
	WaitSemaphoreInfo.semaphore = *Frame->ImageAvailable;
	WaitSemaphoreInfo.stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;

	// this is opposite this is for presentation engine it needs to know when GPU has finished rendering current image
	// so it can present to monitor safely prevents thing like presentation engine display image while gpu drawing into
	// weird :( it does not wait for specific operations it just signaled setting semaphore to signaled
	vk::SemaphoreSubmitInfo RenderFinishedSemaphoreInfo{};
	RenderFinishedSemaphoreInfo.semaphore = *Frame->RenderFinished;
	RenderFinishedSemaphoreInfo.stageMask = vk::PipelineStageFlagBits2::eAllGraphics;

	vk::SubmitInfo2 SubmitInfo{};

	SubmitInfo.commandBufferInfoCount = 1;
	SubmitInfo.pCommandBufferInfos = &CmdBufferSubmitInfo;

	// wait semaphore GPU waits for that to be signaled
	SubmitInfo.waitSemaphoreInfoCount = 1;
	SubmitInfo.pWaitSemaphoreInfos = &WaitSemaphoreInfo;

	// GPU does not it just signal when it hits stageMask
	SubmitInfo.signalSemaphoreInfoCount = 1;
	SubmitInfo.pSignalSemaphoreInfos = &RenderFinishedSemaphoreInfo;

	// async call to submit vulkan will process things internally and send to GPU note: the specs says that
	// command buffer from executable state (vk::CmdEnd) can be transition to pending by queue submission command
	// in pending state the application "must not modify command buffer" (never forget about it) as it may be executing
	// on device
	if (auto Result = Context->GpuDevice.GraphicsQueue.submit2(SubmitInfo, Frame->InFlight); !Result.has_value())
		spdlog::info("Failed to submit queue");

	auto SwapchainHandle = *SwapchainContext.Handle;

	vk::SwapchainPresentFenceInfoKHR PresentFenceInfo{};
	PresentFenceInfo.swapchainCount = 1;
	PresentFenceInfo.pFences = &*Frame->PresentFence;

	vk::PresentInfoKHR PresentInfo{};
	PresentInfo.pNext = &PresentFenceInfo;
	PresentInfo.swapchainCount = 1;
	PresentInfo.pSwapchains = &SwapchainHandle;
	PresentInfo.waitSemaphoreCount = 1;
	// wait for render_finished to be signaled in submit info it will happen when gpu reaches eAllGraphics stage
	// after than the current image has been rendered and presentation engine can finally sent to OS
	PresentInfo.pWaitSemaphores = &*Frame->RenderFinished;
	PresentInfo.pImageIndices = &CurrentImage;

	VkPresentInfoKHR PresentInfoC = static_cast<VkPresentInfoKHR>(PresentInfo);

	VkResult PresentResult = Context->GpuDevice.Device.getDispatcher()->vkQueuePresentKHR(
	    *Context->GpuDevice.PresentationQueue, &PresentInfoC);

	auto Result = static_cast<vk::Result>(PresentResult);

	if (Result != vk::Result::eSuccess && Result != vk::Result::eSuboptimalKHR &&
	    Result != vk::Result::eErrorOutOfDateKHR)
	{
		spdlog::info("Failed to present");
	}

	CurrentFrame = (CurrentFrame + 1) % MaxFrameInFlight;
}
