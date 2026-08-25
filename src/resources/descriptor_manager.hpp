#pragma once

#include "config.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/vulkan.hpp"
#include <span>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

namespace wind::vulkan {
using DescriptorHandle = uint32_t;

class DescriptorManager
{
public:
  WIND_NODISCARD static auto create(const vk::raii::Device& device, vk::DescriptorType descriptor_type) WIND_NOEXCEPT
      -> WindResult<DescriptorManager>;
  WIND_NODISCARD auto create_set(const vk::raii::Device& device) WIND_NOEXCEPT -> WindResult<void>;
  WIND_NODISCARD auto create_layout(const vk::raii::Device& device, std::span<const vk::DescriptorSetLayoutBinding> bindings) WIND_NOEXCEPT
      -> WindResult<void>;

  WIND_NODISCARD auto get_pool() const -> const vk::raii::DescriptorPool*;
  WIND_NODISCARD auto get_set() const -> const vk::raii::DescriptorSet*;
  WIND_NODISCARD auto get_layout() const -> const vk::raii::DescriptorSetLayout*;

private:
  explicit DescriptorManager(vk::raii::DescriptorPool pool)
      : m_pool{std::move(pool)} {};

  vk::raii::DescriptorPool      m_pool{nullptr};
  vk::raii::DescriptorSet       m_set{nullptr};
  vk::raii::DescriptorSetLayout m_layout{nullptr};
};

};  // namespace wind::vulkan
