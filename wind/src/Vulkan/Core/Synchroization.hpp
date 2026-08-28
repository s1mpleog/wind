#pragma once

#include <vulkan/vulkan.hpp>

WIND_INLINE auto transition_image(vk::raii::CommandBuffer& cmd, VkImage image, vk::ImageLayout old_layout, vk::ImageLayout new_layout) WIND_NOEXCEPT
    -> void
{
  // undefined = I don't care what was there before.
  // TransferDstOptimal = I'm about to write pixels through a transfer.
  // ShaderReadOnly = I'm finished writing; shader will read pixels.
  if(old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal)
  {
    vk::ImageMemoryBarrier2 barrier{};
    barrier.image            = image;
    barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;

    // i will not pretend that i understand these
    barrier.srcStageMask  = vk::PipelineStageFlagBits2::eTopOfPipe;
    barrier.srcAccessMask = {};

    barrier.dstStageMask  = vk::PipelineStageFlagBits2::eTransfer;
    barrier.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;

    vk::DependencyInfo dep_info{};
    dep_info.imageMemoryBarrierCount = 1;
    dep_info.pImageMemoryBarriers    = &barrier;

    cmd.pipelineBarrier2(dep_info);
    return;
  }

  if(old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
  {
    vk::ImageMemoryBarrier2 barrier{};
    barrier.image            = image;
    barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;

    // i will not pretend that i understand these
    barrier.srcStageMask  = vk::PipelineStageFlagBits2::eTransfer;
    barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;

    barrier.dstStageMask  = vk::PipelineStageFlagBits2::eFragmentShader;
    barrier.dstAccessMask = vk::AccessFlagBits2::eShaderSampledRead;

    vk::DependencyInfo dep_info{};
    dep_info.imageMemoryBarrierCount = 1;
    dep_info.pImageMemoryBarriers    = &barrier;

    cmd.pipelineBarrier2(dep_info);
    return;
  }

  if(old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eDepthAttachmentOptimal)
  {
    vk::ImageMemoryBarrier2 barrier{};
    barrier.image            = image;
    barrier.subresourceRange = {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1};

    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;

    // i will not pretend that i understand these
    barrier.srcStageMask  = {};
    barrier.srcAccessMask = {};

    barrier.dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
    barrier.dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite;

    vk::DependencyInfo dep_info{};
    dep_info.imageMemoryBarrierCount = 1;
    dep_info.pImageMemoryBarriers    = &barrier;

    cmd.pipelineBarrier2(dep_info);
    return;
  }

  if(old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eColorAttachmentOptimal)
  {
    // transition from undefined to color optimal
    // COLOR_ATTACHMENT_OPTIMAL is the image layout appropriate for the image usage around
    // color-attachment operation

    // so let source side = undefined
    // let destination side = color_attachment_optimal


    //NOTE: for me later instead of thinking about vulkan ENUMS and all think about three things
    // source: GPU is doing X to the image (must happen before)
    // destination: GPU/external system doing Y to the image
    // image: old layout -> new layout
    // source must happen before destination like gpu need to complete X before destination can do Y

    vk::ImageMemoryBarrier2 barrier{};
    // which image is the target
    barrier.image            = image;
    barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

    barrier.oldLayout = vk::ImageLayout::eUndefined;
    barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;

    // why because we can think undefined layout as saying i don't care about what it was previously
    // its irrelevant there is no previous gpu memory access that we need to wait on
    barrier.srcStageMask  = vk::PipelineStageFlagBits2::eNone;
    barrier.srcAccessMask = vk::AccessFlagBits2::eNone;

    // where does that upcoming use occurs ? we know that its related to color attachment
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    // its a write access GPU will write it to change the underlying layout of vkImage from undefined
    // to something that is suitable for color attachment operations
    barrier.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;

    vk::DependencyInfo dep_info{};
    dep_info.imageMemoryBarrierCount = 1;
    dep_info.pImageMemoryBarriers    = &barrier;

    cmd.pipelineBarrier2(dep_info);

    return;
  }

  if(old_layout == vk::ImageLayout::eColorAttachmentOptimal && new_layout == vk::ImageLayout::ePresentSrcKHR)
  {
    vk::ImageMemoryBarrier2 barrier{};
    barrier.image            = image;
    barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

    barrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
    barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;

    // source: GPU rendering writes final color to image
    // operation is write and final color is color_attachment_output in pipeline so,
    barrier.srcStageMask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;

    // destination is presentation engine its not ordinary vulkan pipeline stage
    barrier.dstStageMask  = vk::PipelineStageFlagBits2::eNone;
    barrier.dstAccessMask = vk::AccessFlagBits2::eNone;

    vk::DependencyInfo dep_info{};
    dep_info.imageMemoryBarrierCount = 1;
    dep_info.pImageMemoryBarriers    = &barrier;

    cmd.pipelineBarrier2(dep_info);
  }

  if(old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::ePresentSrcKHR)
  {
    // transition undefined image to swapchain representable format
    vk::ImageMemoryBarrier2 barrier{};
    barrier.image = image;

    barrier.oldLayout = vk::ImageLayout::eUndefined;
    barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;

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
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    // what memory operation is that source operation performing
    // srcAccessMask -> which kind of access to memory ?
    // so here swapchain needs to read from the image to send to presentation layer
    // why ? because GPU is writing to that image while that image is being used as color attachment
    // so, srcAccessMask = what kind of resource access ?
    barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;

    // presentation is not a general vulkan concept think it as presentation is outside of vulkan stage
    // vulkan hands the swapchain image to presentation/display system we don't know what it will do with it
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eNone;
    // what kind of memory/resource access are we synchronizing towards on destination side
    // presentation is not a normal operation that vulkan graphics/compute/transfer specifies right
    // conceptually its outside of general GPU operation its more towards OS and framebuffer and monitor
    barrier.dstAccessMask = vk::AccessFlagBits2::eNone;

    vk::DependencyInfo dep_info{};
    dep_info.imageMemoryBarrierCount = 1;
    dep_info.pImageMemoryBarriers    = &barrier;

    // its a gpu side operation image transition so we have to record it into command buffer
    // as i know vulkan explicitly separates "recording" and "drawing"
    cmd.pipelineBarrier2(dep_info);


    return;
  }
}
