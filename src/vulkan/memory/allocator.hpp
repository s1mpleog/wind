#pragma once

#include "config.hpp"
#include "spdlog/spdlog.h"
#include "utils/expected_util.hpp"
#include "vulkan/core/context.hpp"
#include "vulkan/graphics/pipeline_config.hpp"
#include <ranges>
#include <span>
#include <type_traits>
#include <utility>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

namespace wind::vulkan::memory {
struct AllocatedBuffer
{
  VkBuffer      buffer{};
  VmaAllocation allocation{};
  VmaAllocator  allocator{};

  AllocatedBuffer()                                          = default;
  AllocatedBuffer(const AllocatedBuffer&)                    = delete;
  auto operator=(const AllocatedBuffer&) -> AllocatedBuffer& = delete;

  AllocatedBuffer(VkBuffer buffer, VmaAllocation allocation, VmaAllocator allocator)
      : buffer(buffer)
      , allocation(allocation)
      , allocator(allocator)
  {
  }

  AllocatedBuffer(AllocatedBuffer&& other) noexcept
      : buffer(std::exchange(other.buffer, VK_NULL_HANDLE))
      , allocation(std::exchange(other.allocation, nullptr))
      , allocator(other.allocator) {};

  auto reset() WIND_NOEXCEPT -> void
  {
    if(buffer != VK_NULL_HANDLE)
    {
      vmaDestroyBuffer(allocator, buffer, allocation);

      buffer     = VK_NULL_HANDLE;
      allocation = nullptr;
    }
  }

  auto operator=(AllocatedBuffer&& other) noexcept -> AllocatedBuffer&
  {
    if(this != &other)
    {
      reset();

      buffer     = std::exchange(other.buffer, VK_NULL_HANDLE);
      allocation = std::exchange(other.allocation, nullptr);
      allocator  = std::exchange(other.allocator, nullptr);
    }

    return *this;
  }

  ~AllocatedBuffer()
  {
    if(buffer != VK_NULL_HANDLE)
    {
      vmaDestroyBuffer(allocator, buffer, allocation);
      spdlog::info("cleanup buffer");
    }
  }
};

class GpuAllocator
{
public:
  GpuAllocator(const GpuAllocator&)                    = delete;
  auto operator=(const GpuAllocator&) -> GpuAllocator& = delete;

  GpuAllocator(GpuAllocator&& other) WIND_NOEXCEPT : m_allocator(std::exchange(other.m_allocator, VK_NULL_HANDLE)) {}

  auto operator=(GpuAllocator&& other) WIND_NOEXCEPT->GpuAllocator&
  {
    if(this != &other)
    {
      if(m_allocator != VK_NULL_HANDLE)
        vmaDestroyAllocator(m_allocator);

      m_allocator = std::exchange(other.m_allocator, VK_NULL_HANDLE);
    }

    return *this;
  }

  ~GpuAllocator()
  {
    if(m_allocator != VK_NULL_HANDLE)
    {
      vmaDestroyAllocator(m_allocator);
      spdlog::info("vma allocator destroyed");
    }
  }

  WIND_NODISCARD static auto create(const VulkanContext& context) WIND_NOEXCEPT -> WindResult<GpuAllocator>;

  WIND_NODISCARD auto upload_staging_buffer(std::span<const std::byte> data) WIND_NOEXCEPT -> WindResult<AllocatedBuffer>;

  // TODO: use my custom types for flags
  WIND_NODISCARD auto create_buffer_bytes(std::span<const std::byte> data,
                                          vk::BufferUsageFlagBits flags = vk::BufferUsageFlagBits::eVertexBuffer) WIND_NOEXCEPT
      -> WindResult<AllocatedBuffer>;

  // TODO: get rid of this
  template <std::ranges::contiguous_range R>
  WIND_NODISCARD auto create_buffer(R&& range, vk::BufferUsageFlagBits flags = vk::BufferUsageFlagBits::eVertexBuffer) WIND_NOEXCEPT
      -> WindResult<AllocatedBuffer>
    requires std::is_trivially_copyable_v<std::ranges::range_value_t<R>>
  {
    auto bytes = std::as_bytes(std::span{range});
    return WIND_TRY(create_buffer_bytes(bytes, flags));
  }

  WIND_NODISCARD auto create_texture(std::span<const std::byte> data, u32 width, u32 height, Format format) WIND_NOEXCEPT
      -> WindResult<u32>;

private:
  explicit GpuAllocator(VmaAllocator allocator)
      : m_allocator{allocator} {};

  VmaAllocator m_allocator{VK_NULL_HANDLE};
};

};  // namespace wind::vulkan::memory
