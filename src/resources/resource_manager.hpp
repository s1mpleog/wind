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
#include "error.hpp"
#include "resources/texture_loader.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/core/context.hpp"
#include "vulkan/memory/allocator.hpp"
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
  u32 generation{};
};

struct Texture;
struct Shader;

using TextureHandle = Handle<Texture>;
using ShaderHandle  = Handle<Shader>;

static_assert(sizeof(TextureHandle) == 8);
static_assert(sizeof(ShaderHandle) == 8);

static_assert(!std::is_same_v<TextureHandle, ShaderHandle>);

class ResourceManager
{
public:
  ResourceManager(const ResourceManager&)                    = delete;
  auto operator=(const ResourceManager&) -> ResourceManager& = delete;

  ResourceManager(ResourceManager&&)                    = default;
  auto operator=(ResourceManager&&) -> ResourceManager& = default;

  WIND_NODISCARD static auto create(const vulkan::VulkanContext& context) WIND_NOEXCEPT -> WindResult<ResourceManager>;

  WIND_NODISCARD auto load_shader(const vk::raii::Device& device, std::string_view shader_path) WIND_NOEXCEPT
      -> WindResult<ShaderHandle>;

  WIND_NODISCARD auto load_texture(const vk::raii::Device& device, std::string_view texture_path) WIND_NOEXCEPT
      -> WindResult<TextureHandle>;

  WIND_NODISCARD auto get_shader(ShaderHandle handle) WIND_NOEXCEPT -> WindResult<vk::raii::ShaderModule*>
  {
    if(handle.index > m_shaders.size())
      WIND_ERR(WindError::internal());

    return &m_shaders[handle.index];
  }

  template <typename T>
  WIND_NODISCARD auto get() WIND_NOEXCEPT -> WindResult<T*>;

private:
  explicit ResourceManager(vulkan::memory::GpuAllocator allocator)
      : m_allocator{std::move(allocator)} {};

  std::vector<asset::WindAsset>                 m_assets;
  std::vector<vk::raii::ShaderModule>           m_shaders;
  std::unordered_map<std::string, ShaderHandle> m_shader_cache;
  vulkan::memory::GpuAllocator                  m_allocator;
};

};  // namespace wind::resources
