#include "resource_manager.hpp"
#include "resources/texture_loader.hpp"
#include "spdlog/spdlog.h"
#include "utils/expected_util.hpp"
#include "vulkan/graphics/pipeline_config.hpp"
#include "vulkan/memory/allocator.hpp"
#include "vulkan/vulkan.hpp"
#include <span>

namespace wind::resources {
WIND_NODISCARD auto ResourceManager::create(const vulkan::VulkanContext& context) WIND_NOEXCEPT -> WindResult<ResourceManager>
{
  auto allocator = WIND_TRY(vulkan::memory::GpuAllocator::create(context));
  return ResourceManager{std::move(allocator)};
}

WIND_NODISCARD auto ResourceManager::load_shader(const vk::raii::Device& device, std::string_view shader_path) WIND_NOEXCEPT
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

  u32 index = static_cast<u32>(m_shaders.size());

  m_shaders.emplace_back(WIND_TRY(device.createShaderModule(shader_module_info)));

  ShaderHandle handle{.index = index};

  m_shader_cache.emplace(std::string{shader_path}, handle);

  return handle;
}

WIND_NODISCARD auto ResourceManager::get_shader(ShaderHandle handle) WIND_NOEXCEPT -> WindResult<vk::raii::ShaderModule*>
{
  if(handle.index > m_shaders.size())
    WIND_ERR(WindError::internal());

  return &m_shaders[handle.index];
};

WIND_NODISCARD auto ResourceManager::get_shader_unchecked(ShaderHandle handle) WIND_NOEXCEPT -> vk::raii::ShaderModule*
{
  return &m_shaders[handle.index];
}

// TODO: handle this in better way
auto ResourceManager::destroy_shader(ShaderHandle handle) WIND_NOEXCEPT -> void
{
  if(handle.index > m_shaders.size())
    return;

  m_shaders[handle.index] = nullptr;
}

WIND_NODISCARD auto ResourceManager::load_asset(std::string_view texture_path) WIND_NOEXCEPT -> WindResult<MeshHandle>
{
  auto it = m_asset_cache.find(std::string{texture_path});

  if(it != m_asset_cache.end())
    return it->second;

  auto wind_asset = WIND_TRY(asset::open(texture_path));

  // TODO: instead of two buffers use one buffer
  // |---------------------------|
  // vertex           index
  auto vertices = WIND_TRY(m_allocator.create_buffer(std::move(wind_asset.vertices), vk::BufferUsageFlagBits::eVertexBuffer));
  auto indices = WIND_TRY(m_allocator.create_buffer(std::move(wind_asset.indices), vk::BufferUsageFlagBits::eIndexBuffer));

  auto vertex_index = static_cast<u32>(m_vertex_buffer.size());
  // index_index yeah :(
  auto index_index = static_cast<u32>(m_index_buffer.size());

  m_vertex_buffer.push_back(std::move(vertices));
  m_index_buffer.push_back(std::move(indices));

  spdlog::info("buffer created successfully");

  // temporary
  auto format = [](u32 format) -> vulkan::Format {
    switch(format)
    {
      case 7:
        return vulkan::Format::BC7_UNORM_BLOCK;
      case 5:
        return vulkan::Format::BC5_UNORM_BLOCK;

      default:
        return vulkan::Format::RGBA32_FLOAT;
    }
  };

  for(const auto& texture : wind_asset.textures)
  {
    spdlog::info("width: {}, height: {}, format: {}", texture.width, texture.height, texture.format);
    WIND_TRY(m_allocator.create_texture(std::as_bytes(std::span{texture.data}), texture.width, texture.height,
                                        format(texture.format)));
  }

  for(const auto& material : wind_asset.materials)
  {
    // process materials
  }

  return MeshHandle{.vertex_handle = VertexHandle{vertex_index}, .index_handle = IndexHandle{index_index}};
}

};  // namespace wind::resources
