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
#include "spdlog/spdlog.h"
#include "utils/expected_util.hpp"
#include "vulkan/vulkan.hpp"
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace wind {

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
  //TODO: constrain T
  template <typename T>
  WIND_NODISCARD auto load(std::string_view path) WIND_NOEXCEPT -> WindResult<Handle<T>>
  {
    auto asset = WIND_TRY(asset::load_asset(path));

    // do validation, cache check loading and all

    return Handle<T>{.index = 1, .generation = 0};
  }

  WIND_NODISCARD auto load_shader(const vk::raii::Device& device, std::string_view shader_path) WIND_NOEXCEPT
      -> WindResult<ShaderHandle>
  {
    auto it = m_shader_cache.find(std::string{shader_path});

    if(it != m_shader_cache.end())
      return it->second;

    // cache miss create new shader

    auto path = std::filesystem::path(shader_path);

    if(!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path))
      WIND_ERR(WindError::internal(ErrorCode::FailedToLoadShader));

    auto file_size = std::filesystem::file_size(path);

    if(file_size == 0)
      WIND_ERR(WindError::internal(ErrorCode::FailedToLoadShader));

    if(file_size % sizeof(u32) != 0)
      WIND_ERR(WindError::internal());

    std::ifstream file_stream(path, std::ios::binary);

    if(!file_stream.is_open())
      WIND_ERR(WindError::internal());

    std::vector<u32> buffer(file_size / sizeof(u32));

    if(!file_stream.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(file_size)))
      WIND_ERR(WindError::internal());

    vk::ShaderModuleCreateInfo shader_module_info{};
    shader_module_info.codeSize = buffer.size() * sizeof(u32);
    shader_module_info.pCode    = buffer.data();

    spdlog::info("code size: {}", buffer.size() * 4);

    u32 index = static_cast<u32>(m_shaders.size());

    m_shaders.emplace_back(WIND_TRY(device.createShaderModule(shader_module_info)));

    ShaderHandle handle{.index = index, .generation = 0};

    m_shader_cache.emplace(std::string{shader_path}, handle);

    return handle;
  };

  WIND_NODISCARD auto get_shader(ShaderHandle handle) WIND_NOEXCEPT -> WindResult<vk::raii::ShaderModule*>
  {
    if(handle.index > m_shaders.size())
      WIND_ERR(WindError::internal());

    return &m_shaders[handle.index];
  }

  template <typename T>
  WIND_NODISCARD auto get() WIND_NOEXCEPT -> WindResult<T*>;

private:
  std::vector<asset::WindAsset>                 m_assets;
  std::vector<vk::raii::ShaderModule>           m_shaders;
  std::unordered_map<std::string, ShaderHandle> m_shader_cache;
};

};  // namespace wind
