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
#include "utils/expected_util.hpp"
#include "vulkan/core/context.hpp"
#include "vulkan/memory/allocator.hpp"
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
  VertexHandle vertex_handle{};
  IndexHandle  index_handle{};
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

  // just for testing
  WIND_NODISCARD auto create_vertices(std::span<const std::byte> vertices) WIND_NOEXCEPT -> WindResult<vulkan::memory::AllocatedBuffer>
  {
    auto allocated_buffer = WIND_TRY(m_allocator.create_buffer(m_context, vertices));
    return allocated_buffer;
  }

private:
  ResourceManager(vulkan::memory::GpuAllocator allocator, const vulkan::VulkanContext* context)
      : m_context{context}
      , m_allocator{std::move(allocator)} {};

  const vulkan::VulkanContext*                  m_context;
  std::vector<vk::raii::ShaderModule>           m_shaders;
  std::unordered_map<std::string, ShaderHandle> m_shader_cache;
  vulkan::memory::GpuAllocator                  m_allocator;
  std::vector<vulkan::memory::AllocatedBuffer>  m_vertex_buffer;
  std::vector<vulkan::memory::AllocatedBuffer>  m_index_buffer;
  std::unordered_map<std::string, MeshHandle>   m_asset_cache;
  std::vector<vulkan::memory::AllocatedTexture> m_texture;
  vulkan::memory::AllocatedImage                m_depth_image;
};

};  // namespace wind::resources
