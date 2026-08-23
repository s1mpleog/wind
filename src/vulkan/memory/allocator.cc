#include "allocator.hpp"
#include "error.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"

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

WIND_NODISCARD auto GpuAllocator::create_buffer(vk::DeviceSize size) WIND_NOEXCEPT -> WindResult<u32>
{
  vk::BufferCreateInfo buffer_info{};
  buffer_info.size        = size;
  buffer_info.sharingMode = vk::SharingMode::eExclusive;
  buffer_info.usage       = vk::BufferUsageFlagBits::eTransferSrc;

  // first upload to temporary buffer where GPU and CPU can both access the data then
  // upload it to GPU Vram
  VmaAllocationCreateInfo allocation_info{};
  allocation_info.usage = VMA_MEMORY_USAGE_AUTO;
  allocation_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

  return 0;
}

};  // namespace wind::vulkan::memory
