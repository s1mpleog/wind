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

#include "Config.hpp"
#include "Resources/DescriptorManager.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Context.hpp"
#include "Vulkan/Memory/Allocator.hpp"
#include "Vulkan/Memory/ResourceTypes.hpp"
#include <vulkan/vulkan.hpp>
#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

template <typename T>
struct Handle
{
  u32 index{};
};

struct Texture;
struct Shader;

using TextureHandle       = Handle<Texture>;
using ShaderHandle        = Handle<Shader>;
using VertexHandle        = Handle<u32>;
using IndexHandle         = Handle<u32>;
using ModelHandle         = Handle<u32>;
using BufferHandle        = Handle<u32>;
using DynamicBufferHandle = Handle<u32>;

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

  WIND_NODISCARD static auto create(const VulkanContext* context) WIND_NOEXCEPT -> WindResult<ResourceManager>;

  WIND_NODISCARD auto load_shader(const vk::raii::Device& device, std::string_view shader_path) WIND_NOEXCEPT
      -> WindResult<ShaderHandle>;
  WIND_NODISCARD auto get_shader(ShaderHandle handle) WIND_NOEXCEPT -> WindResult<vk::raii::ShaderModule*>;
  WIND_NODISCARD auto get_shader_unchecked(ShaderHandle handle) WIND_NOEXCEPT -> vk::raii::ShaderModule*;
  auto                destroy_shader(ShaderHandle handle) WIND_NOEXCEPT -> void;

  WIND_NODISCARD auto create_default_depth_image(u32 width, u32 height) WIND_NOEXCEPT -> WindResult<void>;
  WIND_NODISCARD auto get_default_depth_image_view() const WIND_NOEXCEPT -> const vk::raii::ImageView&;

  WIND_NODISCARD auto load_model(std::string_view texture_path) WIND_NOEXCEPT -> WindResult<ModelHandle>;
  //TODO: WindResult does not works with reference fix that for now return pointer
  WIND_NODISCARD auto get_model(ModelHandle handle) WIND_NOEXCEPT -> WindResult<const Model*>;
  WIND_NODISCARD auto get_model_unchecked(ModelHandle handle) WIND_NOEXCEPT -> const Model*;

  WIND_NODISCARD auto recreate_default_depth_image(u32 width, u32 height) WIND_NOEXCEPT -> WindResult<void>;

  WIND_NODISCARD auto create_vertex_buffer(std::span<const std::byte> vertices) WIND_NOEXCEPT -> WindResult<BufferHandle>;
  WIND_NODISCARD auto create_index_buffer(std::span<const std::byte> indices) -> WindResult<BufferHandle>;
  WIND_NODISCARD auto get_buffer(BufferHandle handle) const -> WindResult<const AllocatedBuffer*>;
  WIND_NODISCARD auto get_buffer_unchecked(BufferHandle handle) const -> const AllocatedBuffer*;

  WIND_NODISCARD auto create_dynamic_buffer(u32 size, vk::BufferUsageFlagBits usage = vk::BufferUsageFlagBits::eUniformBuffer) WIND_NOEXCEPT
      -> WindResult<DynamicBufferHandle>;

  WIND_NODISCARD auto create_dynamic_uniform_buffer(u32 size) WIND_NOEXCEPT -> WindResult<DynamicBufferHandle>;

  WIND_NODISCARD auto get_mapped_data(DynamicBufferHandle handle) WIND_NOEXCEPT -> WindResult<void*>;
  WIND_NODISCARD auto get_mapped_data_unchecked(DynamicBufferHandle handle) WIND_NOEXCEPT -> void*;

  WIND_NODISCARD auto get_bindless_descriptor_set() WIND_NOEXCEPT -> const vk::raii::DescriptorSet*
  {
    return m_descriptor_manager.get_set();
  }

  WIND_NODISCARD auto get_bindless_descriptor_layout() WIND_NOEXCEPT -> const vk::raii::DescriptorSetLayout*
  {
    return m_descriptor_manager.get_layout();
  }

private:
  ResourceManager(const VulkanContext* context, GpuAllocator allocator, DescriptorManager descriptor_manager)
      : m_context{context}
      , m_allocator{std::move(allocator)}
      , m_descriptor_manager{std::move(descriptor_manager)} {};

  const VulkanContext*                          m_context;
  std::vector<vk::raii::ShaderModule>           m_shaders;
  std::unordered_map<std::string, ShaderHandle> m_shader_cache;
  GpuAllocator                                  m_allocator;
  std::unordered_map<std::string, ModelHandle>  m_model_cache;
  std::vector<AllocatedTexture>                 m_texture;
  AllocatedImage                                m_depth_image;
  DescriptorManager                             m_descriptor_manager;
  std::vector<Model>                            m_models;
  std::vector<AllocatedBuffer>                  m_buffers;  // for things that don't uses models
};
