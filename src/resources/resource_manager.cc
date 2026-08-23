#include "resource_manager.hpp"
#include "vulkan/memory/allocator.hpp"

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

  ShaderHandle handle{.index = index, .generation = 0};

  m_shader_cache.emplace(std::string{shader_path}, handle);

  return handle;
}
};  // namespace wind::resources
