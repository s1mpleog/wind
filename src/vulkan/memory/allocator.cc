#include "allocator.hpp"
#include "config.hpp"
#include "error.hpp"
#include "spdlog/spdlog.h"
#include "utils/expected_util.hpp"
#include "vulkan/graphics/pipeline_config.hpp"
#include "vulkan/memory/resource_types.hpp"
#include "vulkan/types.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_to_string.hpp>

namespace wind::vulkan::memory {
WIND_NODISCARD auto GpuAllocator::create(const VulkanContext* context) WIND_NOEXCEPT -> WindResult<GpuAllocator>
{
  VmaAllocatorCreateInfo allocator_info{};

  allocator_info.physicalDevice              = *context->gpu_device.physical_device;
  allocator_info.device                      = *context->gpu_device.device;
  allocator_info.preferredLargeHeapBlockSize = 0;
  allocator_info.instance                    = *context->instance;
  allocator_info.vulkanApiVersion            = context->gpu_device.physical_device_props.apiVersion;

  VmaAllocator temp_allocator{};

  if(auto result = vmaCreateAllocator(&allocator_info, &temp_allocator); result != VK_SUCCESS)
    WIND_ERR(WindError::vulkan(ErrorCode::InternalError, static_cast<vk::Result>(result)));

#ifdef WIND_LOG_ENABLE
  spdlog::info("VMA allocator created successfully");
#endif

  vk::CommandBufferAllocateInfo command_buffer_alloc_info{};
  command_buffer_alloc_info.commandBufferCount = 1;
  command_buffer_alloc_info.level              = vk::CommandBufferLevel::ePrimary;
  // use transfer pool if available otherwise use fallback to graphics pool
  command_buffer_alloc_info.commandPool =
      context->gpu_device.has_transfer_queue() ? context->gpu_device.transfer_pool.value() : context->gpu_device.graphics_pool;

  auto command_buffers = WIND_TRY(context->gpu_device.device.allocateCommandBuffers(command_buffer_alloc_info),
                                  ErrorCode::FailedToAllocateCommandBuffer);

  auto fence = WIND_TRY(context->gpu_device.device.createFence(vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled}),
                        ErrorCode::FailedToCreateFence);

  return GpuAllocator{temp_allocator, std::move(command_buffers[0]), std::move(fence)};
}

// TODO: this does not belongs here
auto GpuAllocator::transition_image(vk::raii::CommandBuffer& cmd, VkImage& image, vk::ImageLayout old_layout, vk::ImageLayout new_layout) WIND_NOEXCEPT
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
}

WIND_NODISCARD auto GpuAllocator::upload_staging_buffer(std::span<const std::byte> data) WIND_NOEXCEPT -> WindResult<gpu::AllocatedBuffer>
{
  const vk::DeviceSize size = data.size();

  vk::BufferCreateInfo buffer_info{};
  buffer_info.size        = size;
  buffer_info.sharingMode = vk::SharingMode::eExclusive;
  buffer_info.usage       = vk::BufferUsageFlagBits::eTransferSrc;

  // first create a staging buffer which CPU can access it and write to it
  VmaAllocationCreateInfo allocation_create_info{};
  allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
  allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

  VmaAllocationInfo alloc_info{};
  VkBuffer          buffer{};
  VmaAllocation     allocation{};

  // creates a new vkBuffer, allocates and binds memory for it
  if(auto result = vmaCreateBuffer(m_allocator, buffer_info, &allocation_create_info, &buffer, &allocation, &alloc_info);
     result != VK_SUCCESS)
    WIND_ERR(WindError::vulkan(ErrorCode::FailedToCreateBuffer, static_cast<vk::Result>(result)));

  // copy our data to staging buffer
  std::memcpy(alloc_info.pMappedData, data.data(), size);

  return gpu::AllocatedBuffer{
      buffer,
      allocation,
      m_allocator,
  };
}

WIND_NODISCARD auto GpuAllocator::begin_command_buffer() WIND_NOEXCEPT -> WindResult<void>
{
  WIND_ASSERT(m_command_buffer != nullptr && "Command buffer is null");

  vk::CommandBufferBeginInfo begin_info{};
  begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
  WIND_TRY(m_command_buffer.begin(begin_info), ErrorCode::FailedToBeginCommandBuffer);

  return {};
}

WIND_NODISCARD auto GpuAllocator::end_command_buffer() WIND_NOEXCEPT -> WindResult<void>
{
  WIND_ASSERT(m_command_buffer != nullptr && "Command buffer is null");

  WIND_TRY(m_command_buffer.end());
  return {};
}

WIND_NODISCARD auto GpuAllocator::wait_for_fence(const vk::raii::Device& device) WIND_NOEXCEPT -> WindResult<void>
{
  WIND_ASSERT(m_fence != nullptr && "fence is null");
  auto result = device.waitForFences(*m_fence, vk::True, UINT64_MAX);

  if(result != vk::Result::eSuccess)
    WIND_ERR(WindError::vulkan(ErrorCode::FailedToWaitForFence, result));

  return {};
}


WIND_NODISCARD auto GpuAllocator::reset_fence(const vk::raii::Device& device) WIND_NOEXCEPT -> WindResult<void>
{
  WIND_TRY(device.resetFences(*m_fence));
  return {};
}

WIND_NODISCARD auto GpuAllocator::create_buffer(const VulkanContext*       context,
                                                std::span<const std::byte> data,
                                                vk::BufferUsageFlagBits flags) WIND_NOEXCEPT -> WindResult<gpu::AllocatedBuffer>
{
  WIND_ASSERT(m_allocator != VK_NULL_HANDLE && "VMA allocator is null");
  WIND_ASSERT(context != VK_NULL_HANDLE && "context is null");

  const vk::DeviceSize size = data.size();

  // the idea is that first upload the buffer to a memory location where gpu can cpu can both read and write
  // it will be slow for gpu to access tho its called staging buffer so allocate a staging buffer and write
  // vertices there then create another buffer that will be local_bit means cpu can not write to its the fastest
  // for gpu and copy the buffer there through PCIe
  // TlDr: staging buffer is for HOST (CPU) and final buffer is for GPU

  auto staging_buffer_info = WIND_TRY(upload_staging_buffer(data));

  spdlog::info("staging buffer created: {}", (void*)staging_buffer_info.buffer);

  // now actual buffer CPU can't able to write to this only GPU can it will be on VRAM

  vk::BufferCreateInfo buffer_create_info{};
  buffer_create_info.size = size;

  // transferDst because we are copying from staging buffer and eVertexBuffer because we want to use this
  // as what ever flags is
  vk::BufferUsageFlags usage     = flags | vk::BufferUsageFlagBits::eTransferDst;
  buffer_create_info.usage       = usage;
  buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

  // prefer device aka GPU
  VmaAllocationCreateInfo allocation_create_info{};
  allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  VmaAllocation allocation{};
  VkBuffer      device_buffer{};

  if(auto result = vmaCreateBuffer(m_allocator, buffer_create_info, &allocation_create_info, &device_buffer, &allocation, nullptr);
     result != VK_SUCCESS)
    WIND_ERR(WindError::vulkan(ErrorCode::FailedToCreateBuffer, static_cast<vk::Result>(result)));

  spdlog::info("buffer created: {}", (void*)device_buffer);

  // ================Upload the data to GPU===============

  // set command buffer to recording state
  WIND_TRY(begin_command_buffer());

  vk::BufferCopy copy_region{};
  copy_region.size      = size;
  copy_region.dstOffset = 0;
  copy_region.srcOffset = 0;

  // copy the buffer
  m_command_buffer.copyBuffer(staging_buffer_info.buffer, device_buffer, copy_region);

  //set command buffer to executable state
  WIND_TRY(end_command_buffer());

  vk::SubmitInfo submit_info{};
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers    = &*m_command_buffer;

  // wait for previous submission to complete since we created m_fence in signaled state
  // on first call it will just return aka it will not hang during first call
  // otherwise it will wait for the previous buffer submission to finished regardless of that
  // the new state of m_fence will be SIGNALED
  WIND_TRY(wait_for_fence(context->gpu_device.device));
  // reset_fence changes m_fence from signaled to unsigned we need this because vulkan says
  // that we should no submit using signaled fence state: UN-SIGNALED
  WIND_TRY(reset_fence(context->gpu_device.device));

  if(context->gpu_device.has_transfer_queue())
  {
    WIND_TRY(context->gpu_device.transfer_queue->submit(submit_info, m_fence), ErrorCode::FailedToSubmitQueue);
  }
  else
  {
    WIND_TRY(context->gpu_device.graphics_queue.submit(submit_info, m_fence), ErrorCode::FailedToSubmitQueue);
  }

  // why are we waiting here also umm is it because of staging buffer cleanup and stopping
  // RAII invocation until GPU process staging buffer ? i think so cause queue.submit()
  // is async call so we need a synchronization
  WIND_TRY(wait_for_fence(context->gpu_device.device));

  return gpu::AllocatedBuffer{device_buffer, allocation, m_allocator};
}

WIND_NODISCARD auto GpuAllocator::create_vk_image(u32 width, u32 height, Format format, vk::ImageUsageFlags usage) WIND_NOEXCEPT
    -> WindResult<std::pair<VkImage, VmaAllocation>>
{
  vk::ImageCreateInfo image_create_info{};
  image_create_info.imageType     = vk::ImageType::e2D;
  image_create_info.extent        = {.width = width, .height = height, .depth = 1};
  image_create_info.format        = to_vk(format);
  image_create_info.tiling        = vk::ImageTiling::eOptimal;
  image_create_info.samples       = vk::SampleCountFlagBits::e1;
  image_create_info.usage         = usage;
  image_create_info.mipLevels     = 1;
  image_create_info.arrayLayers   = 1;
  image_create_info.initialLayout = vk::ImageLayout::eUndefined;

  spdlog::info(
      "creating image: {}x{}, format={}, tiling={}, usage={:#x}, "
      "mipLevels={}, arrayLayers={}, samples={}",
      width, height, vk::to_string(image_create_info.format), vk::to_string(image_create_info.tiling),
      static_cast<VkImageUsageFlags>(image_create_info.usage), image_create_info.mipLevels,
      image_create_info.arrayLayers, vk::to_string(image_create_info.samples));

  VkImageCreateInfo vk_image_create_info = image_create_info;

  VmaAllocationCreateInfo image_allocation_info{};
  image_allocation_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  VmaAllocation image_allocation{};
  VkImage       image{};

  if(auto result = vmaCreateImage(m_allocator, &vk_image_create_info, &image_allocation_info, &image, &image_allocation, nullptr);
     result != VK_SUCCESS)
    WIND_ERR(WindError::vulkan(ErrorCode::FailedToCreateImage, static_cast<vk::Result>(result)));

  return std::pair{image, image_allocation};
}

WIND_NODISCARD auto GpuAllocator::create_texture(const VulkanContext*       context,
                                                 std::span<const std::byte> pixels,
                                                 u32                        width,
                                                 u32                        height,
                                                 Format format) WIND_NOEXCEPT -> WindResult<gpu::AllocatedTexture>
{
  WIND_ASSERT(m_allocator != VK_NULL_HANDLE && "VMA allocator is null");
  WIND_ASSERT(context != VK_NULL_HANDLE && "context is null");

  auto staging_buffer = WIND_TRY(upload_staging_buffer(pixels));

  auto [image, image_allocation] =
      WIND_TRY(create_vk_image(width, height, format, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled));

  spdlog::info("image created successfully: {}", (void*)image);

  vk::ImageViewCreateInfo image_view_create_info{};
  image_view_create_info.image            = image;
  image_view_create_info.format           = to_vk(format);
  image_view_create_info.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
  image_view_create_info.viewType         = vk::ImageViewType::e2D;

  // create image view
  auto image_view = WIND_TRY(context->gpu_device.device.createImageView(image_view_create_info));

  spdlog::info("image_view: {}", (void*)*image_view);

  // ================Upload the data to GPU===============
  WIND_TRY(begin_command_buffer());

  vk::BufferImageCopy region{};
  region.imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1};
  region.imageExtent      = {.width = width, .height = height, .depth = 1};

  // transition from undefined to TransferDst
  transition_image(m_command_buffer, image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

  m_command_buffer.copyBufferToImage(staging_buffer.buffer, image, vk::ImageLayout::eTransferDstOptimal, region);

  // transition from Transfer dst to shader read
  transition_image(m_command_buffer, image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

  // end the command buffer no more recording :(
  WIND_TRY(end_command_buffer());

  vk::SubmitInfo submit_info{};
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers    = &*m_command_buffer;

  // return iff STATE == SIGNALED
  WIND_TRY(wait_for_fence(context->gpu_device.device));
  // reset state to UN-SIGNALED
  WIND_TRY(reset_fence(context->gpu_device.device));

  if(context->gpu_device.has_transfer_queue())
  {
    WIND_TRY(context->gpu_device.transfer_queue->submit(submit_info, *m_fence), ErrorCode::FailedToSubmitQueue);
  }
  else
  {
    WIND_TRY(context->gpu_device.graphics_queue.submit(submit_info, *m_fence), ErrorCode::FailedToSubmitQueue);
  }

  WIND_TRY(wait_for_fence(context->gpu_device.device));

  vk::SamplerCreateInfo sampler_create_info{};
  sampler_create_info.anisotropyEnable = vk::False;
  sampler_create_info.magFilter        = vk::Filter::eLinear;
  sampler_create_info.minFilter        = vk::Filter::eLinear;
  sampler_create_info.mipmapMode       = vk::SamplerMipmapMode::eLinear;
  sampler_create_info.addressModeU     = vk::SamplerAddressMode::eRepeat;
  sampler_create_info.addressModeV     = vk::SamplerAddressMode::eRepeat;
  sampler_create_info.addressModeW     = vk::SamplerAddressMode::eRepeat;

  auto sampler = WIND_TRY(context->gpu_device.device.createSampler(sampler_create_info));

  return gpu::AllocatedTexture{gpu::AllocatedImage{image, std::move(image_view), m_allocator, image_allocation,
                                                   to_vk(format), height, width},
                               std::move(sampler)};
}

WIND_NODISCARD auto GpuAllocator::create_depth_buffer(const VulkanContext* context, u32 width, u32 height) WIND_NOEXCEPT
    -> WindResult<gpu::AllocatedImage>
{
  auto [image, image_allocation] =
      WIND_TRY(create_vk_image(width, height, Format::D32_FLOAT, vk::ImageUsageFlagBits::eDepthStencilAttachment));

  vk::ImageViewCreateInfo image_view_create_info{};
  image_view_create_info.image            = image;
  image_view_create_info.format           = to_vk(Format::D32_FLOAT);
  image_view_create_info.subresourceRange = {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1};
  image_view_create_info.viewType         = vk::ImageViewType::e2D;

  // create image view
  auto image_view = WIND_TRY(context->gpu_device.device.createImageView(image_view_create_info));

  // ================Upload the data to GPU===============

  // set command buffer to recording state
  WIND_TRY(begin_command_buffer());

  // transition from undefined dst to depth optimal
  transition_image(m_command_buffer, image, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal);

  // end the command buffer no more recording :(
  WIND_TRY(end_command_buffer());

  vk::SubmitInfo submit_info{};
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers    = &*m_command_buffer;

  WIND_TRY(wait_for_fence(context->gpu_device.device));
  WIND_TRY(reset_fence(context->gpu_device.device));

  if(context->gpu_device.has_transfer_queue())
  {
    WIND_TRY(context->gpu_device.transfer_queue->submit(submit_info, *m_fence), ErrorCode::FailedToSubmitQueue);
  }
  else
  {
    WIND_TRY(context->gpu_device.graphics_queue.submit(submit_info, *m_fence), ErrorCode::FailedToSubmitQueue);
  }

  WIND_TRY(wait_for_fence(context->gpu_device.device));

  return gpu::AllocatedImage{
      image, std::move(image_view), m_allocator, image_allocation, to_vk(Format::D32_FLOAT), height, width};
}

};  // namespace wind::vulkan::memory
