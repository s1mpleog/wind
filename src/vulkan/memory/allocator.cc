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
#include <ranges>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_to_string.hpp>
#include "vulkan/core/synchroization.hpp"

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

WIND_NODISCARD auto GpuAllocator::create_buffers(const VulkanContext* context, std::span<const gpu::BufferData> buffers) WIND_NOEXCEPT
    -> WindResult<std::vector<gpu::AllocatedBuffer>>
{
  WIND_ASSERT(m_allocator != VK_NULL_HANDLE && "VMA allocator is null");
  WIND_ASSERT(context != VK_NULL_HANDLE && "context is null");

  std::vector<gpu::AllocatedBuffer> allocated_buffers;
  allocated_buffers.reserve(buffers.size());

  std::vector<gpu::AllocatedBuffer> staging_buffers;
  staging_buffers.reserve(buffers.size());

  WIND_TRY(begin_command_buffer());

  WIND_TRY(wait_for_fence(context->gpu_device.device));
  WIND_TRY(reset_fence(context->gpu_device.device));

  for(auto&& [index, buffer] : std::views::enumerate(buffers))
  {
    const vk::DeviceSize size = buffer.data.size();
    staging_buffers.emplace_back(WIND_TRY(upload_staging_buffer(buffer.data)));

    spdlog::info("staging buffer created: {}", (void*)staging_buffers[index].buffer);

    vk::BufferCreateInfo buffer_create_info{};
    buffer_create_info.size = size;

    // transferDst because we are copying from staging buffer and eVertexBuffer because we want to use this
    // as what ever flags is
    vk::BufferUsageFlags usage     = buffer.usage | vk::BufferUsageFlagBits::eTransferDst;
    buffer_create_info.usage       = usage;
    buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

    VmaAllocationCreateInfo allocation_create_info{};
    allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    VmaAllocation image_allocation{};
    VkBuffer      device_buffer{};

    if(auto result = vmaCreateBuffer(m_allocator, buffer_create_info, &allocation_create_info, &device_buffer, &image_allocation, nullptr);
       result != VK_SUCCESS)
    {
      WIND_ERR(WindError::vulkan(ErrorCode::FailedToCreateBuffer, static_cast<vk::Result>(result)));
    }

    spdlog::info("buffer created: {}", (void*)device_buffer);

    vk::BufferCopy copy_region{};
    copy_region.size      = size;
    copy_region.dstOffset = 0;
    copy_region.srcOffset = 0;

    // copy the buffer
    m_command_buffer.copyBuffer(staging_buffers[index].buffer, device_buffer, copy_region);

    allocated_buffers.emplace_back(device_buffer, image_allocation, m_allocator);
  }

  WIND_TRY(end_command_buffer());

  vk::SubmitInfo submit_info{};
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers    = &*m_command_buffer;

  if(context->gpu_device.has_transfer_queue())
  {
    WIND_TRY(context->gpu_device.transfer_queue->submit(submit_info, *m_fence), ErrorCode::FailedToSubmitQueue);
  }
  else
  {
    WIND_TRY(context->gpu_device.graphics_queue.submit(submit_info, *m_fence), ErrorCode::FailedToSubmitQueue);
  }

  WIND_TRY(wait_for_fence(context->gpu_device.device));

  return allocated_buffers;
}

WIND_NODISCARD auto GpuAllocator::create_buffer(const VulkanContext* context, const gpu::BufferData& buffer) WIND_NOEXCEPT
    -> WindResult<gpu::AllocatedBuffer>
{
  auto result = WIND_TRY(create_buffers(context, std::span{&buffer, 1}));
  WIND_ENSURE_NOT_EMPTY(result, WindError::vulkan());

  return std::move(result.front());
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

WIND_NODISCARD auto GpuAllocator::create_texture(const VulkanContext* context, std::span<const gpu::TextureData> texture_data) WIND_NOEXCEPT
    -> WindResult<std::vector<gpu::AllocatedTexture>>
{
  WIND_ASSERT(m_allocator != VK_NULL_HANDLE && "VMA allocator is null");
  WIND_ASSERT(context != VK_NULL_HANDLE && "context is null");

  std::vector<gpu::AllocatedBuffer> staging_buffers;
  staging_buffers.reserve(texture_data.size());

  std::vector<gpu::AllocatedTexture> textures;
  textures.reserve(texture_data.size());

  WIND_TRY(begin_command_buffer());

  // return iff STATE == SIGNALED
  WIND_TRY(wait_for_fence(context->gpu_device.device));
  // reset state to UN-SIGNALED
  WIND_TRY(reset_fence(context->gpu_device.device));

  for(auto&& [index, data] : std::views::enumerate(texture_data))
  {
    // we can do that since we have to create staging buffer for each data
    staging_buffers.emplace_back(WIND_TRY(upload_staging_buffer(data.pixels)));

    auto [image, image_allocation] = WIND_TRY(create_vk_image(data.dimensions.width, data.dimensions.height, data.format));

    spdlog::info("image created successfully: {}", (void*)image);

    vk::ImageViewCreateInfo image_view_create_info{};
    image_view_create_info.image            = image;
    image_view_create_info.format           = to_vk(data.format);
    image_view_create_info.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
    image_view_create_info.viewType         = vk::ImageViewType::e2D;

    // create image view
    auto image_view = WIND_TRY(context->gpu_device.device.createImageView(image_view_create_info));

    vk::BufferImageCopy region{};
    region.imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1};
    region.imageExtent = {.width = data.dimensions.width, .height = data.dimensions.height, .depth = data.dimensions.depth};

    // transition from undefined to TransferDst
    sync::transition_image(m_command_buffer, image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

    m_command_buffer.copyBufferToImage(staging_buffers[index].buffer, image, vk::ImageLayout::eTransferDstOptimal, region);

    // transition from Transfer dst to shader read
    sync::transition_image(m_command_buffer, image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

    vk::SamplerCreateInfo sampler_create_info{};
    sampler_create_info.anisotropyEnable = vk::False;
    sampler_create_info.magFilter        = vk::Filter::eLinear;
    sampler_create_info.minFilter        = vk::Filter::eLinear;
    sampler_create_info.mipmapMode       = vk::SamplerMipmapMode::eLinear;
    sampler_create_info.addressModeU     = vk::SamplerAddressMode::eRepeat;
    sampler_create_info.addressModeV     = vk::SamplerAddressMode::eRepeat;
    sampler_create_info.addressModeW     = vk::SamplerAddressMode::eRepeat;

    auto sampler = WIND_TRY(context->gpu_device.device.createSampler(sampler_create_info));

    textures.emplace_back(gpu::AllocatedImage{image, std::move(image_view), m_allocator, image_allocation, to_vk(data.format),
                                              vk::Extent2D{
                                                  data.dimensions.width,
                                                  data.dimensions.height,
                                              }},
                          std::move(sampler));
  }

  WIND_TRY(end_command_buffer());

  vk::SubmitInfo submit_info{};
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers    = &*m_command_buffer;

  if(context->gpu_device.has_transfer_queue())
  {
    WIND_TRY(context->gpu_device.transfer_queue->submit(submit_info, *m_fence), ErrorCode::FailedToSubmitQueue);
  }
  else
  {
    WIND_TRY(context->gpu_device.graphics_queue.submit(submit_info, *m_fence), ErrorCode::FailedToSubmitQueue);
  }

  WIND_TRY(wait_for_fence(context->gpu_device.device));

  return textures;
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
  sync::transition_image(m_command_buffer, image, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal);

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

  return gpu::AllocatedImage{image,
                             std::move(image_view),
                             m_allocator,
                             image_allocation,
                             to_vk(Format::D32_FLOAT),
                             vk::Extent2D{width, height}};
}

};  // namespace wind::vulkan::memory
