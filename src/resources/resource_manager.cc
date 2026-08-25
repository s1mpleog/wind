#include "resource_manager.hpp"
#include "config.hpp"
#include "error.hpp"
#include "resources/texture_loader.hpp"
#include "spdlog/spdlog.h"
#include "utils/expected_util.hpp"
#include "vulkan/graphics/pipeline_config.hpp"
#include "vulkan/memory/allocator.hpp"
#include "vulkan/memory/resource_types.hpp"
#include "vulkan/vulkan.hpp"
#include <ranges>
#include <span>
#include <utility>
#include <vector>

namespace wind::resources {
WIND_NODISCARD auto ResourceManager::create(const vulkan::VulkanContext* context) WIND_NOEXCEPT -> WindResult<ResourceManager>
{
  auto allocator = WIND_TRY(vulkan::memory::GpuAllocator::create(context));
  return ResourceManager{std::move(allocator), context};
}

WIND_NODISCARD auto ResourceManager::load_shader(const vk::raii::Device& device, std::string_view shader_path) WIND_NOEXCEPT
    -> WindResult<ShaderHandle>
{
  auto it = m_shader_cache.find(std::string{shader_path});

  spdlog::info("context in resource manager: {}", (void*)m_context);

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

  auto mesh = gpu::Mesh{};

  auto wind_asset = WIND_TRY(asset::open(texture_path));

  // TODO: instead of two buffers use one buffer
  // |---------------------------|
  // vertex           index
  mesh.vertex_buffer = WIND_TRY(m_allocator.create_buffer(m_context, gpu::BufferData{
                                                                         .data = std::as_bytes(std::span{wind_asset.vertices}),
                                                                         .usage = vk::BufferUsageFlagBits::eVertexBuffer,
                                                                     }));

  mesh.index_buffer = WIND_TRY(m_allocator.create_buffer(m_context, gpu::BufferData{
                                                                        .data = std::as_bytes(std::span{wind_asset.indices}),
                                                                        .usage = vk::BufferUsageFlagBits::eIndexBuffer,
                                                                    }));

  mesh.vertex_count = static_cast<u32>(wind_asset.vertices.size());

  mesh.index_count = static_cast<u32>(wind_asset.indices.size());

  auto mesh_index = static_cast<u32>(m_meshes.size());

  m_meshes.push_back(std::move(mesh));

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

  m_texture.reserve(wind_asset.textures.size());

  std::vector<gpu::TextureData> texture_data;
  texture_data.reserve(wind_asset.textures.size());

  for(const auto&& [index, texture] : std::views::enumerate(wind_asset.textures))
  {
    auto data = gpu::TextureData{
        .pixels     = std::as_bytes(std::span{texture.data}),
        .dimensions = {texture.width, texture.height, 1},
        .format     = format(texture.format),
    };

    texture_data.push_back(data);
  }

  m_texture = WIND_TRY(m_allocator.create_texture(m_context, texture_data));

  spdlog::info("created textures: {}", m_texture.size());

  for(const auto& material : wind_asset.materials)
  {
    // process materials
  }

  return MeshHandle{.index = mesh_index};
}

WIND_NODISCARD auto ResourceManager::create_depth_image(u32 width, u32 height) WIND_NOEXCEPT -> WindResult<void>
{
  m_depth_image = WIND_TRY(m_allocator.create_depth_buffer(m_context, width, height));
  return {};
}

WIND_NODISCARD auto ResourceManager::get_depth_image_view() const WIND_NOEXCEPT -> const vk::raii::ImageView&
{
  WIND_ASSERT(m_depth_image.image_view != nullptr && "Depth image view is nullptr create depth image first");
  return m_depth_image.image_view;
}

WIND_NODISCARD auto ResourceManager::get_mesh(MeshHandle handle) WIND_NOEXCEPT -> WindResult<const gpu::Mesh*>
{
  if(handle.index > m_meshes.size())
    WIND_ERR(WindError::internal());

  return &m_meshes[handle.index];
}

WIND_NODISCARD auto ResourceManager::get_mesh_unchecked(MeshHandle handle) WIND_NOEXCEPT -> const gpu::Mesh*
{
  return &m_meshes[handle.index];
}

};  // namespace wind::resources
