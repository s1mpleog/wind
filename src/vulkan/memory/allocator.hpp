#pragma once

#include "config.hpp"
#include "spdlog/spdlog.h"
#include "utils/expected_util.hpp"
#include "vulkan/core/context.hpp"
#include "vulkan/graphics/pipeline_config.hpp"
#include "vulkan/vulkan.hpp"
#include <span>
#include <utility>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

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
      spdlog::info("VkBuffer destroyed successfully");
    }
  }
};


struct AllocatedImage
{
  VkImage             image{VK_NULL_HANDLE};
  vk::raii::ImageView image_view{nullptr};
  VmaAllocator        allocator{VK_NULL_HANDLE};
  VmaAllocation       allocation{VK_NULL_HANDLE};
  vk::Format          format{};
  u32                 width{};
  u32                 height{};

  AllocatedImage(VkImage image, vk::raii::ImageView image_view, VmaAllocator allocator, VmaAllocation allocation, vk::Format format, u32 height, u32 width)
      : image{image}
      , image_view{std::move(image_view)}
      , allocator{allocator}
      , allocation{allocation}
      , format{format}
      , width{width}
      , height{height}
  {
  }

  AllocatedImage()                                         = default;
  AllocatedImage(const AllocatedImage&)                    = delete;
  auto operator=(const AllocatedImage&) -> AllocatedImage& = delete;

  AllocatedImage(AllocatedImage&& other) WIND_NOEXCEPT : image{std::exchange(other.image, VK_NULL_HANDLE)},
                                                         image_view{std::move(other.image_view)},
                                                         allocator{std::exchange(other.allocator, VK_NULL_HANDLE)},
                                                         allocation{std::exchange(other.allocation, VK_NULL_HANDLE)},
                                                         format{other.format},
                                                         width{other.width},
                                                         height{other.height}
  {
  }

  auto operator=(AllocatedImage&& other) WIND_NOEXCEPT->AllocatedImage&
  {
    if(this == &other)
      return *this;

    // Destroy our current VMA-owned image first.
    if(allocation != VK_NULL_HANDLE && allocator != VK_NULL_HANDLE)
    {
      vmaDestroyImage(allocator, image, allocation);
    }

    image      = std::exchange(other.image, VK_NULL_HANDLE);
    allocation = std::exchange(other.allocation, VK_NULL_HANDLE);
    allocator  = std::exchange(other.allocator, VK_NULL_HANDLE);

    image_view = std::move(other.image_view);

    format = other.format;
    width  = other.width;
    height = other.height;

    return *this;
  }

  ~AllocatedImage()
  {
    if(allocation != VK_NULL_HANDLE && allocator != VK_NULL_HANDLE)
    {
      vmaDestroyImage(allocator, image, allocation);
      spdlog::info("texture destroyed successfully");
    }
  }
};

struct AllocatedTexture
{
  AllocatedImage    image{};
  vk::raii::Sampler sampler{nullptr};

  AllocatedTexture(AllocatedImage image, vk::raii::Sampler sampler)
      : image{std::move(image)}
      , sampler{std::move(sampler)}
  {
  }

  AllocatedTexture(const AllocatedTexture&)                    = delete;
  auto operator=(const AllocatedTexture&) -> AllocatedTexture& = delete;

  AllocatedTexture(AllocatedTexture&&) WIND_NOEXCEPT                  = default;
  auto operator=(AllocatedTexture&&) WIND_NOEXCEPT->AllocatedTexture& = default;

  ~AllocatedTexture() = default;
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

  WIND_NODISCARD static auto create(const VulkanContext* context) WIND_NOEXCEPT -> WindResult<GpuAllocator>;

  WIND_NODISCARD auto upload_staging_buffer(std::span<const std::byte> data) WIND_NOEXCEPT -> WindResult<AllocatedBuffer>;

  // TODO: use my custom types for flags
  WIND_NODISCARD auto create_buffer(const VulkanContext*       context,
                                    std::span<const std::byte> data,
                                    vk::BufferUsageFlagBits flags = vk::BufferUsageFlagBits::eVertexBuffer) WIND_NOEXCEPT
      -> WindResult<AllocatedBuffer>;

  // TODO: get rid of this
  // template <std::ranges::contiguous_range R>
  // WIND_NODISCARD auto create_buffer(R&& range, vk::BufferUsageFlagBits flags = vk::BufferUsageFlagBits::eVertexBuffer) WIND_NOEXCEPT
  //     -> WindResult<AllocatedBuffer>
  //   requires std::is_trivially_copyable_v<std::ranges::range_value_t<R>>
  // {
  //   auto bytes = std::as_bytes(std::span{range});
  //   return WIND_TRY(create_buffer_bytes(bytes, flags));
  // }

  //TODO: this will go somewhere else
  auto transition_image(vk::raii::CommandBuffer& cmd, VkImage& image, vk::ImageLayout old_layout, vk::ImageLayout new_layout) WIND_NOEXCEPT
      -> void;

  WIND_NODISCARD auto create_texture(const VulkanContext* context, std::span<const std::byte> pixels, u32 width, u32 height, Format format) WIND_NOEXCEPT
      -> WindResult<AllocatedTexture>;

  WIND_NODISCARD auto create_vk_image(u32                 width,
                                      u32                 height,
                                      Format              format,
                                      vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eTransferDst
                                                                  | vk::ImageUsageFlagBits::eSampled) WIND_NOEXCEPT
      -> WindResult<std::pair<VkImage, VmaAllocation>>;

  WIND_NODISCARD auto create_depth_buffer(const VulkanContext* context, u32 width, u32 height) WIND_NOEXCEPT
      -> WindResult<AllocatedImage>;

private:
  explicit GpuAllocator(VmaAllocator allocator)
      : m_allocator{allocator} {};

  VmaAllocator m_allocator{VK_NULL_HANDLE};
};
};  // namespace wind::vulkan::memory
