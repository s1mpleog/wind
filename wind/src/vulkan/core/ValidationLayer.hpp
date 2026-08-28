#pragma once

#include "utils/ExpectedUtil.hpp"
#include "vulkan/core/Configuration.hpp"
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

namespace wind::vulkan {
WIND_INLINE auto debug_callback(vk::DebugUtilsMessageSeverityFlagBitsEXT           message_severity,
                                [[maybe_unused]] vk::DebugUtilsMessageTypeFlagsEXT message_type,
                                const vk::DebugUtilsMessengerCallbackDataEXT*      p_callback_data,
                                [[maybe_unused]] void*                             p_user_data) -> vk::Bool32
{
  if(message_severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
    spdlog::error("[VK] {}", p_callback_data->pMessage);
  else if(message_severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
    spdlog::warn("[VK] {}", p_callback_data->pMessage);
  else if(message_severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo)
    spdlog::info("[VK] {}", p_callback_data->pMessage);

  return VK_FALSE;
}

auto create_debug_utils(const Configuration& cfg, const vk::raii::Instance& instance) WIND_NOEXCEPT
    -> WindResult<vk::raii::DebugUtilsMessengerEXT>;

};  // namespace wind::vulkan
