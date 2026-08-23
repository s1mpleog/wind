#pragma once

#include "config.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/core/context.hpp"
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

namespace wind::vulkan::memory {

class GpuAllocator
{
public:
  GpuAllocator(const GpuAllocator&)                    = delete;
  auto operator=(const GpuAllocator&) -> GpuAllocator& = delete;

  GpuAllocator(GpuAllocator&&)                    = default;
  auto operator=(GpuAllocator&&) -> GpuAllocator& = default;

  WIND_NODISCARD static auto create(const VulkanContext& context) WIND_NOEXCEPT -> WindResult<GpuAllocator>;

  WIND_NODISCARD auto create_buffer(vk::DeviceSize size) WIND_NOEXCEPT -> WindResult<u32>;

private:
  explicit GpuAllocator(VmaAllocator allocator)
      : m_allocator{allocator} {};

  VmaAllocator m_allocator;
};

};  // namespace wind::vulkan::memory
