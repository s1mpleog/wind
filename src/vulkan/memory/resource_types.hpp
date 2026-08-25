#pragma once

#include "vulkan/graphics/pipeline_config.hpp"
#include "vulkan/vulkan.hpp"
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>

namespace wind::gpu {
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

struct Mesh
{
  AllocatedBuffer vertex_buffer;
  AllocatedBuffer index_buffer;

  u32 index_count{};
  u32 vertex_count{};
};

struct GpuMaterial
{
  std::optional<u32> albedo_idx;
  std::optional<u32> normal_idx;
  std::optional<u32> metallic_roughness_idx;
  float              metallic{};
  float              roughness{};
  glm::vec4          base_color{};
};

struct AllocatedImage
{
  VkImage             image{VK_NULL_HANDLE};
  vk::raii::ImageView image_view{nullptr};
  VmaAllocator        allocator{VK_NULL_HANDLE};
  VmaAllocation       allocation{VK_NULL_HANDLE};
  vk::Format          format{};
  vk::Extent2D        dimension{};

  AllocatedImage(VkImage image, vk::raii::ImageView image_view, VmaAllocator allocator, VmaAllocation allocation, vk::Format format, vk::Extent2D dimension)
      : image{image}
      , image_view{std::move(image_view)}
      , allocator{allocator}
      , allocation{allocation}
      , format{format}
      , dimension{dimension}
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
                                                         dimension{other.dimension}
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

    format    = other.format;
    dimension = other.dimension;

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

struct TextureData
{
  std::span<const std::byte> pixels;
  vk::Extent3D               dimensions{};
  vulkan::Format             format{};
};

struct BufferData
{
  std::span<const std::byte> data;
  vk::BufferUsageFlags       usage{vk::BufferUsageFlagBits::eVertexBuffer};
};

};  // namespace wind::gpu
