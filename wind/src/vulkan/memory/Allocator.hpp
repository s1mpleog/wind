#pragma once

#include "Config.hpp"
#include "spdlog/spdlog.h"
#include "utils/ExpectedUtil.hpp"
#include "vulkan/core/Context.hpp"
#include "vulkan/graphics/PipelineConfig.hpp"
#include "vulkan/memory/ResourceTypes.hpp"
#include "vulkan/vulkan.hpp"
#include <cstddef>
#include <span>
#include <utility>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

namespace wind::vulkan::memory {

class GpuAllocator
{
public:
  GpuAllocator(const GpuAllocator&)                    = delete;
  auto operator=(const GpuAllocator&) -> GpuAllocator& = delete;

  GpuAllocator(GpuAllocator&& other) WIND_NOEXCEPT : m_allocator{std::exchange(other.m_allocator, VK_NULL_HANDLE)},
                                                     m_command_buffer{std::move(other.m_command_buffer)},
                                                     m_fence{std::move(other.m_fence)}
  {
  }

  auto operator=(GpuAllocator&& other) WIND_NOEXCEPT->GpuAllocator&
  {
    if(this != &other)
    {
      if(m_allocator != VK_NULL_HANDLE)
        vmaDestroyAllocator(m_allocator);

      m_allocator = std::exchange(other.m_allocator, VK_NULL_HANDLE);

      m_command_buffer = std::move(other.m_command_buffer);
      m_fence          = std::move(other.m_fence);
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

  WIND_NODISCARD static auto create(const VulkanContext* context) WIND_NOEXCEPT -> WindResult<GpuAllocator>;
  WIND_NODISCARD auto upload_staging_buffer(std::span<const std::byte> data) WIND_NOEXCEPT -> WindResult<gpu::AllocatedBuffer>;

  // TODO: use my custom types for flags
  WIND_NODISCARD auto create_buffers(const VulkanContext* context, std::span<const gpu::BufferData> buffers) WIND_NOEXCEPT
      -> WindResult<std::vector<gpu::AllocatedBuffer>>;

  WIND_NODISCARD auto create_buffer(const VulkanContext* context, const gpu::BufferData& buffer) WIND_NOEXCEPT
      -> WindResult<gpu::AllocatedBuffer>;

  // TODO: maybe take ownership of data instead of view ?
  WIND_NODISCARD auto create_texture(const VulkanContext* context, std::span<const gpu::TextureData> texture_data) WIND_NOEXCEPT
      -> WindResult<std::vector<gpu::AllocatedTexture>>;

  WIND_NODISCARD auto create_vk_image(u32                 width,
                                      u32                 height,
                                      Format              format,
                                      vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eTransferDst
                                                                  | vk::ImageUsageFlagBits::eSampled) WIND_NOEXCEPT
      -> WindResult<std::pair<VkImage, VmaAllocation>>;

  WIND_NODISCARD auto create_depth_buffer(const VulkanContext* context, u32 width, u32 height) WIND_NOEXCEPT
      -> WindResult<gpu::AllocatedImage>;

  WIND_NODISCARD auto begin_command_buffer() WIND_NOEXCEPT -> WindResult<void>;
  WIND_NODISCARD auto end_command_buffer() WIND_NOEXCEPT -> WindResult<void>;
  WIND_NODISCARD auto wait_for_fence(const vk::raii::Device& device) WIND_NOEXCEPT -> WindResult<void>;
  WIND_NODISCARD auto reset_fence(const vk::raii::Device& device) WIND_NOEXCEPT -> WindResult<void>;
  auto                is_fence_signaled(const vk::raii::Device& device) WIND_NOEXCEPT -> bool;

  WIND_NODISCARD auto create_dynamic_buffer(u32 size, vk::BufferUsageFlagBits usage = vk::BufferUsageFlagBits::eUniformBuffer) WIND_NOEXCEPT
      -> WindResult<gpu::AllocatedBuffer>;

private:
  GpuAllocator(VmaAllocator allocator, vk::raii::CommandBuffer command_buffer, vk::raii::Fence fence)
      : m_allocator{allocator}
      , m_command_buffer{std::move(command_buffer)}
      , m_fence{std::move(fence)} {};

  VmaAllocator            m_allocator{VK_NULL_HANDLE};
  vk::raii::CommandBuffer m_command_buffer{nullptr};
  vk::raii::Fence         m_fence{nullptr};
};

};  // namespace wind::vulkan::memory
