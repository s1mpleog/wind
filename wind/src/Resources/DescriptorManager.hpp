#pragma once

#include "Config.hpp"
#include "spdlog/spdlog.h"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Memory/ResourceTypes.hpp"
#include <vulkan/vulkan.hpp>
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

  WIND_NODISCARD auto register_texture(const vk::raii::Device& device, const gpu::AllocatedTexture& texture) WIND_NOEXCEPT -> u32
  {
    vk::DescriptorImageInfo image_info{};
    image_info.imageView   = texture.image.image_view;
    image_info.sampler     = texture.sampler;
    image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    const u32 index = m_index++;

    spdlog::info("current index for bindless descriptor set: {}", index);

    vk::WriteDescriptorSet write{};
    write.descriptorCount = 1;
    write.descriptorType  = vk::DescriptorType::eCombinedImageSampler;
    write.dstBinding      = 0;
    write.dstArrayElement = index;
    write.pImageInfo      = &image_info;
    write.dstSet          = m_set;

    device.updateDescriptorSets(write, {});

    return index;
  }

private:
  explicit DescriptorManager(vk::raii::DescriptorPool pool)
      : m_pool{std::move(pool)} {};

  vk::raii::DescriptorPool      m_pool{nullptr};
  vk::raii::DescriptorSet       m_set{nullptr};
  vk::raii::DescriptorSetLayout m_layout{nullptr};
  u32                           m_index{};
};

};  // namespace wind::vulkan
