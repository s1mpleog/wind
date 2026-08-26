#pragma once

#include "vulkan/vulkan.hpp"

namespace wind::vulkan::sync {
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

  if(old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::ePresentSrcKHR)
  {
    // transition undefined image to swapchain representable format
  }
}

};  // namespace wind::vulkan::sync
