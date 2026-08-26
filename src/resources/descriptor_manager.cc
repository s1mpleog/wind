#include "descriptor_manager.hpp"
#include "config.hpp"
#include "spdlog/spdlog.h"
#include "utils/expected_util.hpp"
#include "vulkan/vulkan.hpp"
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_to_string.hpp>

namespace wind::vulkan {
WIND_NODISCARD auto DescriptorManager::create(const vk::raii::Device& device, vk::DescriptorType descriptor_type) WIND_NOEXCEPT
    -> WindResult<DescriptorManager>
{
  // todo: don't use luck number
  vk::DescriptorPoolSize pool_size{};
  pool_size.descriptorCount = 1024;
  pool_size.type            = descriptor_type;

  vk::DescriptorPoolCreateInfo ds_pool_create_info{};
  ds_pool_create_info.flags         = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
  ds_pool_create_info.pPoolSizes    = &pool_size;
  ds_pool_create_info.poolSizeCount = 1;
  ds_pool_create_info.maxSets       = 1024;

  auto pool = WIND_TRY(device.createDescriptorPool(ds_pool_create_info));

#ifdef WIND_LOG_ENABLE
  spdlog::info("created {} descriptor pools with layout: {}", pool_size.descriptorCount, vk::to_string(descriptor_type));
#endif

  return DescriptorManager{std::move(pool)};
}

WIND_NODISCARD auto DescriptorManager::create_set(const vk::raii::Device& device) WIND_NOEXCEPT -> WindResult<void>
{
  WIND_ASSERT(m_pool != nullptr && "Descriptor pool is null while trying to allocate");
  WIND_ASSERT(m_layout != VK_NULL_HANDLE && "layout is null oops created layout first");

  vk::DescriptorSetAllocateInfo alloc_info{};
  alloc_info.descriptorPool     = m_pool;
  alloc_info.descriptorSetCount = 1;
  alloc_info.pSetLayouts        = &*m_layout;

  auto result = WIND_TRY(device.allocateDescriptorSets(alloc_info));

#ifdef WIND_LOG_ENABLE
  spdlog::info("descriptor set created successfully");
#endif

  m_set = std::move(result.front());

  return {};
}

WIND_NODISCARD auto DescriptorManager::create_layout(const vk::raii::Device& device,
                                                     std::span<const vk::DescriptorSetLayoutBinding> bindings) WIND_NOEXCEPT
    -> WindResult<void>
{
  vk::DescriptorSetLayoutCreateInfo layout_create_info{};
  layout_create_info.bindingCount = static_cast<u32>(bindings.size());
  layout_create_info.pBindings    = bindings.data();

  m_layout = WIND_TRY(device.createDescriptorSetLayout(layout_create_info));

  return {};
}

WIND_NODISCARD auto DescriptorManager::get_pool() const -> const vk::raii::DescriptorPool*
{
  WIND_ASSERT(m_pool != nullptr && "pool is null");
  return &m_pool;
}

WIND_NODISCARD auto DescriptorManager::get_set() const -> const vk::raii::DescriptorSet*
{
  WIND_ASSERT(m_set != nullptr && "set is nullptr");
  return &m_set;
}

WIND_NODISCARD auto DescriptorManager::get_layout() const -> const vk::raii::DescriptorSetLayout*
{
  WIND_ASSERT(m_layout != nullptr && "layout is nullptr");
  return &m_layout;
}

}  // namespace wind::vulkan
