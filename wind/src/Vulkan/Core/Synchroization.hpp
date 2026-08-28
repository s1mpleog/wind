#pragma once

#include <vulkan/vulkan.hpp>

WIND_INLINE auto TransitionImage(vk::raii::CommandBuffer &Cmd, VkImage Image, vk::ImageLayout OldLayout,
                                 vk::ImageLayout NewLayout) WIND_NOEXCEPT -> void
{
	// undefined = I don't care what was there before.
	// TransferDstOptimal = I'm about to write pixels through a transfer.
	// ShaderReadOnly = I'm finished writing; shader will read pixels.
	if (OldLayout == vk::ImageLayout::eUndefined && NewLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		vk::ImageMemoryBarrier2 Barrier{};
		Barrier.image = Image;
		Barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

		Barrier.oldLayout = OldLayout;
		Barrier.newLayout = NewLayout;

		// i will not pretend that i understand these
		Barrier.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
		Barrier.srcAccessMask = {};

		Barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
		Barrier.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;

		vk::DependencyInfo DepInfo{};
		DepInfo.imageMemoryBarrierCount = 1;
		DepInfo.pImageMemoryBarriers = &Barrier;

		Cmd.pipelineBarrier2(DepInfo);
		return;
	}

	if (OldLayout == vk::ImageLayout::eTransferDstOptimal && NewLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		vk::ImageMemoryBarrier2 Barrier{};
		Barrier.image = Image;
		Barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

		Barrier.oldLayout = OldLayout;
		Barrier.newLayout = NewLayout;

		// i will not pretend that i understand these
		Barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
		Barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;

		Barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
		Barrier.dstAccessMask = vk::AccessFlagBits2::eShaderSampledRead;

		vk::DependencyInfo DepInfo{};
		DepInfo.imageMemoryBarrierCount = 1;
		DepInfo.pImageMemoryBarriers = &Barrier;

		Cmd.pipelineBarrier2(DepInfo);
		return;
	}

	if (OldLayout == vk::ImageLayout::eUndefined && NewLayout == vk::ImageLayout::eDepthAttachmentOptimal)
	{
		vk::ImageMemoryBarrier2 Barrier{};
		Barrier.image = Image;
		Barrier.subresourceRange = {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1};

		Barrier.oldLayout = OldLayout;
		Barrier.newLayout = NewLayout;

		// i will not pretend that i understand these
		Barrier.srcStageMask = {};
		Barrier.srcAccessMask = {};

		Barrier.dstStageMask =
		    vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
		Barrier.dstAccessMask =
		    vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite;

		vk::DependencyInfo DepInfo{};
		DepInfo.imageMemoryBarrierCount = 1;
		DepInfo.pImageMemoryBarriers = &Barrier;

		Cmd.pipelineBarrier2(DepInfo);
		return;
	}

	if (OldLayout == vk::ImageLayout::eUndefined && NewLayout == vk::ImageLayout::eColorAttachmentOptimal)
	{
		// transition from undefined to color optimal
		// COLOR_ATTACHMENT_OPTIMAL is the image layout appropriate for the image usage around
		// color-attachment operation

		// so let source side = undefined
		// let destination side = color_attachment_optimal

		// NOTE: for me later instead of thinking about vulkan ENUMS and all think about three things
		//  source: GPU is doing X to the image (must happen before)
		//  destination: GPU/external system doing Y to the image
		//  image: old layout -> new layout
		//  source must happen before destination like gpu need to complete X before destination can do Y

		vk::ImageMemoryBarrier2 Barrier{};
		// which image is the target
		Barrier.image = Image;
		Barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

		Barrier.oldLayout = vk::ImageLayout::eUndefined;
		Barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;

		// why because we can think undefined layout as saying i don't care about what it was previously
		// its irrelevant there is no previous gpu memory access that we need to wait on
		Barrier.srcStageMask = vk::PipelineStageFlagBits2::eNone;
		Barrier.srcAccessMask = vk::AccessFlagBits2::eNone;

		// where does that upcoming use occurs ? we know that its related to color attachment
		Barrier.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
		// its a write access GPU will write it to change the underlying layout of vkImage from undefined
		// to something that is suitable for color attachment operations
		Barrier.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;

		vk::DependencyInfo DepInfo{};
		DepInfo.imageMemoryBarrierCount = 1;
		DepInfo.pImageMemoryBarriers = &Barrier;

		Cmd.pipelineBarrier2(DepInfo);

		return;
	}

	if (OldLayout == vk::ImageLayout::eColorAttachmentOptimal && NewLayout == vk::ImageLayout::ePresentSrcKHR)
	{
		vk::ImageMemoryBarrier2 Barrier{};
		Barrier.image = Image;
		Barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

		Barrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
		Barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;

		// source: GPU rendering writes final color to image
		// operation is write and final color is color_attachment_output in pipeline so,
		Barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
		Barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;

		// destination is presentation engine its not ordinary vulkan pipeline stage
		Barrier.dstStageMask = vk::PipelineStageFlagBits2::eNone;
		Barrier.dstAccessMask = vk::AccessFlagBits2::eNone;

		vk::DependencyInfo DepInfo{};
		DepInfo.imageMemoryBarrierCount = 1;
		DepInfo.pImageMemoryBarriers = &Barrier;

		Cmd.pipelineBarrier2(DepInfo);
	}

	if (OldLayout == vk::ImageLayout::eUndefined && NewLayout == vk::ImageLayout::ePresentSrcKHR)
	{
		// transition undefined image to swapchain representable format
		vk::ImageMemoryBarrier2 Barrier{};
		Barrier.image = Image;

		Barrier.oldLayout = vk::ImageLayout::eUndefined;
		Barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;

		// let rendering be the source side and let presentation engine be destination side
		// and let color attachment be the target
		// rendering -> writes to image (actual image processing graphics pipeline rasterization and all)
		// presentation -> reads that image so it can send to underlying os presentation engine to display

		// which pipeline stage(s) contain the operation whose completion i am waiting for
		// this is for swapchain right so we are waiting gpu to finish rendering the image so swapchain
		// can send it to presentation layer for display
		// so rasterization happens then fragment shading happens after than color attachment output happens
		// and at that point we know that gpu has processed that image so we want to wait for color attachment output
		// so, srcStageMask -> which pipeline stage ? (describes source side of dependency)
		Barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
		// what memory operation is that source operation performing
		// srcAccessMask -> which kind of access to memory ?
		// so here swapchain needs to read from the image to send to presentation layer
		// why ? because GPU is writing to that image while that image is being used as color attachment
		// so, srcAccessMask = what kind of resource access ?
		Barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;

		// presentation is not a general vulkan concept think it as presentation is outside of vulkan stage
		// vulkan hands the swapchain image to presentation/display system we don't know what it will do with it
		Barrier.dstStageMask = vk::PipelineStageFlagBits2::eNone;
		// what kind of memory/resource access are we synchronizing towards on destination side
		// presentation is not a normal operation that vulkan graphics/compute/transfer specifies right
		// conceptually its outside of general GPU operation its more towards OS and framebuffer and monitor
		Barrier.dstAccessMask = vk::AccessFlagBits2::eNone;

		vk::DependencyInfo DepInfo{};
		DepInfo.imageMemoryBarrierCount = 1;
		DepInfo.pImageMemoryBarriers = &Barrier;

		// its a gpu side operation image transition so we have to record it into command buffer
		// as i know vulkan explicitly separates "recording" and "drawing"
		Cmd.pipelineBarrier2(DepInfo);

		return;
	}
}
