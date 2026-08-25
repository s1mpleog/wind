#pragma once

// i want it to be like this
// TextureHandle texture = resources<Texture>.load("assets/texture.png")
// ShaderHandle shader = resources<Shader>.load(device, "assets/shader.vert")
// where, Texture and Shader is a handle not underlying resource
// conceptually struct Handle { u32 index; u32 generation }
// provide options like get and delete do validation internally
// flow:
// TextureHandle tex = resource<Texture>.load("...");
// internally validate does file exists ? ...
// check the cache if hit -> increment index return handle
// load the file -> cache it -> increment handle data -> return handle


#include "config.hpp"
#include "resources/descriptor_manager.hpp"
#include "spdlog/spdlog.h"
#include "utils/expected_util.hpp"
#include "vulkan/core/context.hpp"
#include "vulkan/memory/allocator.hpp"
#include "vulkan/memory/resource_types.hpp"
#include "vulkan/vulkan.hpp"
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace wind::resources {

template <typename T>
struct Handle
{
  u32 index{};
};

struct Texture;
struct Shader;

using TextureHandle = Handle<Texture>;
using ShaderHandle  = Handle<Shader>;
using VertexHandle  = Handle<u32>;
using IndexHandle   = Handle<u32>;

static_assert(sizeof(TextureHandle) == 4);
static_assert(sizeof(ShaderHandle) == 4);

static_assert(!std::is_same_v<TextureHandle, ShaderHandle>);

struct MeshHandle
{
  u32 index{};
};

class ResourceManager
{
public:
  ResourceManager(const ResourceManager&)                    = delete;
  auto operator=(const ResourceManager&) -> ResourceManager& = delete;

  ResourceManager(ResourceManager&&)                    = default;
  auto operator=(ResourceManager&&) -> ResourceManager& = default;

  WIND_NODISCARD static auto create(const vulkan::VulkanContext* context) WIND_NOEXCEPT -> WindResult<ResourceManager>;

  WIND_NODISCARD auto load_shader(const vk::raii::Device& device, std::string_view shader_path) WIND_NOEXCEPT
      -> WindResult<ShaderHandle>;
  WIND_NODISCARD auto get_shader(ShaderHandle handle) WIND_NOEXCEPT -> WindResult<vk::raii::ShaderModule*>;
  WIND_NODISCARD auto get_shader_unchecked(ShaderHandle handle) WIND_NOEXCEPT -> vk::raii::ShaderModule*;
  auto                destroy_shader(ShaderHandle handle) WIND_NOEXCEPT -> void;

  WIND_NODISCARD auto load_asset(std::string_view texture_path) WIND_NOEXCEPT -> WindResult<MeshHandle>;

  WIND_NODISCARD auto create_depth_image(u32 width, u32 height) WIND_NOEXCEPT -> WindResult<void>;
  WIND_NODISCARD auto get_depth_image_view() const WIND_NOEXCEPT -> const vk::raii::ImageView&;

  //TODO: WindResult does not works with reference fix that for now return pointer
  WIND_NODISCARD auto get_mesh(MeshHandle handle) WIND_NOEXCEPT -> WindResult<const gpu::Mesh*>;
  WIND_NODISCARD auto get_mesh_unchecked(MeshHandle handle) WIND_NOEXCEPT -> const gpu::Mesh*;

  // just for testing
  WIND_NODISCARD auto create_vertices(std::span<const std::byte> vertices) WIND_NOEXCEPT -> WindResult<gpu::AllocatedBuffer>
  {
    auto allocated_buffer = WIND_TRY(m_allocator.create_buffer(m_context, gpu::BufferData{
                                                                              .data = vertices,
                                                                              .usage = vk::BufferUsageFlagBits::eVertexBuffer,
                                                                          }));
    return allocated_buffer;
  }


  WIND_NODISCARD auto register_texture(const gpu::AllocatedTexture& texture, u32 index) WIND_NOEXCEPT -> void
  {
    vk::DescriptorImageInfo image_info{};
    image_info.imageView   = texture.image.image_view;
    image_info.sampler     = texture.sampler;
    image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    vk::WriteDescriptorSet write{};
    write.descriptorCount = 1;
    write.descriptorType  = vk::DescriptorType::eCombinedImageSampler;
    write.dstBinding      = 0;
    write.dstArrayElement = index;
    write.pImageInfo      = &image_info;
    write.dstSet          = *m_descriptor_manager.get_set();

    m_context->gpu_device.device.updateDescriptorSets(write, {});
  }

private:
  ResourceManager(const vulkan::VulkanContext* context, vulkan::memory::GpuAllocator allocator, vulkan::DescriptorManager descriptor_manager)
      : m_context{context}
      , m_allocator{std::move(allocator)}
      , m_descriptor_manager{std::move(descriptor_manager)} {};

  const vulkan::VulkanContext*                  m_context;
  std::vector<vk::raii::ShaderModule>           m_shaders;
  std::unordered_map<std::string, ShaderHandle> m_shader_cache;
  vulkan::memory::GpuAllocator                  m_allocator;
  std::unordered_map<std::string, MeshHandle>   m_asset_cache;
  std::vector<gpu::Mesh>                        m_meshes;
  std::vector<gpu::AllocatedTexture>            m_texture;
  gpu::AllocatedImage                           m_depth_image;
  vulkan::DescriptorManager                     m_descriptor_manager;
};

};  // namespace wind::resources
