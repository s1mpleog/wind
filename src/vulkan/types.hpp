#pragma once

#include "vulkan/core/configuration.hpp"

namespace wind::vulkan {

constexpr auto to_vk(VulkanVersion value) WIND_NOEXCEPT -> uint32_t
{
  switch(value)
  {
    case VulkanVersion::VERSION13:
      return vk::ApiVersion13;

    case VulkanVersion::VERSION14:
      return vk::ApiVersion14;
  }

  std::unreachable();
}

constexpr auto to_vk(DebugMessageSeverity value) WIND_NOEXCEPT -> vk::DebugUtilsMessageSeverityFlagBitsEXT
{
  return static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(static_cast<u32>(value));
}

constexpr auto to_vk(DebugMessageType value) WIND_NOEXCEPT -> vk::DebugUtilsMessageTypeFlagsEXT
{
  return static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(static_cast<u32>(value));
}

};  // namespace wind::vulkan
