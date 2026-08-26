#include "resource_manager.hpp"
#include "config.hpp"
#include "error.hpp"
#include "resources/descriptor_manager.hpp"
#include "resources/texture_loader.hpp"
#include "spdlog/spdlog.h"
#include "utils/expected_util.hpp"
#include "vulkan/graphics/pipeline_config.hpp"
#include "vulkan/memory/allocator.hpp"
#include "vulkan/memory/resource_types.hpp"
#include "vulkan/vulkan.hpp"
#include <inplace_vector>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

namespace wind::resources {
WIND_NODISCARD auto ResourceManager::create(const vulkan::VulkanContext* context) WIND_NOEXCEPT -> WindResult<ResourceManager>
{
  auto allocator = WIND_TRY(vulkan::memory::GpuAllocator::create(context));

  auto descriptor_manager =
      WIND_TRY(vulkan::DescriptorManager::create(context->gpu_device.device, vk::DescriptorType::eCombinedImageSampler));

  vk::DescriptorSetLayoutBinding binding{};
  // slot 0 in texture
  binding.binding = 0;
  // image and sampler combined
  binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
  // max textures
  binding.descriptorCount = 1024;
  // fragment shader will use it
  binding.stageFlags = vk::ShaderStageFlagBits::eFragment;

  vk::DescriptorSetLayoutBindingFlagsCreateInfo flags_info{};
  // don't need all filled can add texture after binding
  vk::DescriptorBindingFlags binding_flags =
      vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateAfterBind;

  flags_info.bindingCount  = 1;
  flags_info.pBindingFlags = &binding_flags;

  vk::DescriptorSetLayoutCreateInfo layout_info{};
  layout_info.pNext        = &flags_info;
  layout_info.flags        = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;
  layout_info.bindingCount = 1;
  layout_info.pBindings    = &binding;

  WIND_TRY(descriptor_manager.create_layout(context->gpu_device.device, std::span{&binding, 1}));
  WIND_TRY(descriptor_manager.create_set(context->gpu_device.device));

  return ResourceManager{context, std::move(allocator), std::move(descriptor_manager)};
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

WIND_NODISCARD auto ResourceManager::load_model(std::string_view texture_path) WIND_NOEXCEPT -> WindResult<ModelHandle>
{
  auto it = m_model_cache.find(std::string{texture_path});

  if(it != m_model_cache.end())
  {
    spdlog::info("model is already in cache loading");
    return it->second;
  }

  auto mesh = gpu::Mesh{};

  auto model = gpu::Model{};

  auto wind_asset = WIND_TRY(asset::open(texture_path));

  // TODO: instead of two buffers use one buffer
  // |---------------------------|
  // vertex           index

  // mesh.index_buffer = WIND_TRY(m_allocator.create_buffer(m_context, ));

  std::inplace_vector<gpu::BufferData, 4> buffers{{gpu::BufferData{
                                                      .data  = std::as_bytes(std::span{wind_asset.mesh.position}),
                                                      .usage = vk::BufferUsageFlagBits::eVertexBuffer,
                                                  }},
                                                  {gpu::BufferData{
                                                      .data  = std::as_bytes(std::span{wind_asset.mesh.indices}),
                                                      .usage = vk::BufferUsageFlagBits::eIndexBuffer,
                                                  }},
                                                  {gpu::BufferData{
                                                      .data  = std::as_bytes(std::span{wind_asset.mesh.normals}),
                                                      .usage = vk::BufferUsageFlagBits::eVertexBuffer,
                                                  }},
                                                  {gpu::BufferData{
                                                      .data  = std::as_bytes(std::span{wind_asset.mesh.uvs}),
                                                      .usage = vk::BufferUsageFlagBits::eVertexBuffer,
                                                  }}};

  std::inplace_vector<gpu::BufferData, 4> test;

  //TODO: currently .wind fallbacks for uvs and normals so it will always exists
  // but later its not true so handle that with this
  // vertices and indices must exists according to .wind specs
  // buffers.push_back({
  //     .data  = std::as_bytes(std::span{wind_asset.mesh.position}),
  //     .usage = vk::BufferUsageFlagBits::eVertexBuffer,
  // });

  // buffers.push_back({
  //     .data  = std::as_bytes(std::span{wind_asset.mesh.indices}),
  //     .usage = vk::BufferUsageFlagBits::eIndexBuffer,
  // });

  // // normals and uvs can be empty
  // if(!wind_asset.mesh.normals.empty())
  // {
  //   buffers.push_back({
  //       .data  = std::as_bytes(std::span{wind_asset.mesh.normals}),
  //       .usage = vk::BufferUsageFlagBits::eVertexBuffer,
  //   });
  // }

  // if(!wind_asset.mesh.uvs.empty())
  // {
  //   buffers.push_back({
  //       .data  = std::as_bytes(std::span{wind_asset.mesh.uvs}),
  //       .usage = vk::BufferUsageFlagBits::eVertexBuffer,
  //   });
  // }

  auto result = WIND_TRY(m_allocator.create_buffers(m_context, buffers));

  model.mesh.vertex_buffer = std::move(result[0]);
  model.mesh.index_buffer  = std::move(result[1]);
  model.mesh.normals       = std::move(result[2]);
  model.mesh.uvs           = std::move(result[3]);

  auto uvs = WIND_TRY(m_allocator.create_buffer(m_context, gpu::BufferData{
                                                               .data  = std::as_bytes(std::span{wind_asset.mesh.uvs}),
                                                               .usage = vk::BufferUsageFlagBits::eVertexBuffer,
                                                           }));

  model.mesh.vertex_count = static_cast<u32>(wind_asset.mesh.position.size());
  model.mesh.index_count  = static_cast<u32>(wind_asset.mesh.indices.size());
  model.mesh.uv_count     = static_cast<u32>(wind_asset.mesh.uvs.size());
  model.mesh.normal_count = static_cast<u32>(wind_asset.mesh.normals.size());

  // auto mesh_index = static_cast<u32>(m_meshes.size());

  // m_meshes.push_back(std::move(mesh));

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

  // only if texture and material is not empty
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

  model.materials.resize(wind_asset.materials.size());

  for(auto&& [index, material] : std::views::enumerate(wind_asset.materials))
  {
    model.materials[index].roughness  = material.roughness;
    model.materials[index].base_color = {material.base_color[0], material.base_color[1], material.base_color[2],
                                         material.base_color[3]};

    if(material.albedo_index)
    {
      auto& texture = m_texture[*material.albedo_index];
      model.materials[index].albedo_texture = m_descriptor_manager.register_texture(m_context->gpu_device.device, texture);
    }

    if(material.normal_index)
    {
      auto& texture = m_texture[*material.normal_index];
      model.materials[index].normal_texture = m_descriptor_manager.register_texture(m_context->gpu_device.device, texture);
    }

    if(material.metallic_roughness_index)
    {
      auto& texture = m_texture[*material.metallic_roughness_index];
      model.materials[index].metallic_roughness_texture =
          m_descriptor_manager.register_texture(m_context->gpu_device.device, texture);
    }
  }

  u32 model_index = static_cast<u32>(m_models.size());

  auto handle = ModelHandle{.index = model_index};

  // update cache
  m_model_cache.emplace(std::string{texture_path}, handle);

  m_models.push_back(std::move(model));

  return handle;
}

WIND_NODISCARD auto ResourceManager::create_default_depth_image(u32 width, u32 height) WIND_NOEXCEPT -> WindResult<void>
{
  m_depth_image = WIND_TRY(m_allocator.create_depth_buffer(m_context, width, height));
  return {};
}

WIND_NODISCARD auto ResourceManager::get_default_depth_image_view() const WIND_NOEXCEPT -> const vk::raii::ImageView&
{
  WIND_ASSERT(m_depth_image.image_view != nullptr && "Depth image view is nullptr create depth image first");
  return m_depth_image.image_view;
}

WIND_NODISCARD auto ResourceManager::get_model(ModelHandle handle) WIND_NOEXCEPT -> WindResult<const gpu::Model*>
{
  if(handle.index > m_models.size())
    WIND_ERR(WindError::internal());

  return &m_models[handle.index];
}

WIND_NODISCARD auto ResourceManager::get_model_unchecked(ModelHandle handle) WIND_NOEXCEPT -> const gpu::Model*
{
  return &m_models[handle.index];
}


WIND_NODISCARD auto ResourceManager::recreate_default_depth_image(u32 width, u32 height) WIND_NOEXCEPT -> WindResult<void>
{
  // destroy the old depth image
  m_depth_image.destroy_image();

  // create new one
  m_depth_image = WIND_TRY(m_allocator.create_depth_buffer(m_context, width, height));

  return {};
}

};  // namespace wind::resources
