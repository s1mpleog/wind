#include "allocator.hpp"
#include "config.hpp"
#include "error.hpp"
#include "spdlog/spdlog.h"
#include "utils/expected_util.hpp"
#include "vulkan/graphics/pipeline_config.hpp"
#include "vulkan/types.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include <cstddef>
#include <cstring>
#include <span>
#include <vulkan/vulkan_to_string.hpp>

namespace wind::vulkan::memory {
WIND_NODISCARD auto GpuAllocator::create(const VulkanContext& context) WIND_NOEXCEPT -> WindResult<GpuAllocator>
{
  VmaAllocatorCreateInfo allocator_info{};

  allocator_info.physicalDevice              = *context.gpu_device.physical_device;
  allocator_info.device                      = *context.gpu_device.device;
  allocator_info.preferredLargeHeapBlockSize = 0;
  allocator_info.instance                    = *context.instance;
  allocator_info.vulkanApiVersion            = context.gpu_device.physical_device_props.apiVersion;

  VmaAllocator temp_allocator{};

  if(auto result = vmaCreateAllocator(&allocator_info, &temp_allocator); result != VK_SUCCESS)
    WIND_ERR(WindError::vulkan(ErrorCode::InternalError, static_cast<vk::Result>(result)));

#ifdef WIND_LOG_ENABLE
  spdlog::info("VMA allocator created successfully");
#endif

  return GpuAllocator{temp_allocator};
}

WIND_NODISCARD auto GpuAllocator::upload_staging_buffer(std::span<const std::byte> data) WIND_NOEXCEPT -> WindResult<AllocatedBuffer>
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

  return AllocatedBuffer{
      buffer,
      allocation,
      m_allocator,
  };
}

WIND_NODISCARD auto GpuAllocator::create_buffer_bytes(std::span<const std::byte> data, vk::BufferUsageFlagBits flags) WIND_NOEXCEPT
    -> WindResult<AllocatedBuffer>
{
  WIND_ASSERT(m_allocator != VK_NULL_HANDLE && "VMA allocator is null");

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
  VkBuffer      buffer{};

  if(auto result = vmaCreateBuffer(m_allocator, buffer_create_info, &allocation_create_info, &buffer, &allocation, nullptr);
     result != VK_SUCCESS)
    WIND_ERR(WindError::vulkan(ErrorCode::FailedToCreateBuffer, static_cast<vk::Result>(result)));

  spdlog::info("buffer created: {}", (void*)buffer);

  return AllocatedBuffer{buffer, allocation, m_allocator};
}

WIND_NODISCARD auto GpuAllocator::create_texture(std::span<const std::byte> data, u32 width, u32 height, Format format) WIND_NOEXCEPT
    -> WindResult<u32>
{
  WIND_ASSERT(m_allocator != VK_NULL_HANDLE && "VMA allocator is null");
  auto staging_buffer = WIND_TRY(upload_staging_buffer(data));

  vk::ImageCreateInfo image_create_info{};
  image_create_info.imageType   = vk::ImageType::e2D;
  image_create_info.extent      = {.width = width, .height = height, .depth = 1};
  image_create_info.format      = to_vk(format);
  image_create_info.tiling      = vk::ImageTiling::eOptimal;
  image_create_info.samples     = vk::SampleCountFlagBits::e1;
  image_create_info.usage       = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
  image_create_info.mipLevels   = 1;
  image_create_info.arrayLayers = 1;

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
  VkImage       raw_image{};

  if(auto result = vmaCreateImage(m_allocator, &vk_image_create_info, &image_allocation_info, &raw_image, &image_allocation, nullptr);
     result != VK_SUCCESS)
  {
    WIND_ERR(WindError::vulkan(ErrorCode::FailedToCreateImage, static_cast<vk::Result>(result)));
  }

  spdlog::info("image created successfully: {}", (void*)raw_image);

  return 0;
}

};  // namespace wind::vulkan::memory
