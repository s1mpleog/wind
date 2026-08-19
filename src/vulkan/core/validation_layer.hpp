#pragma once

#include "utils/expected_util.hpp"
#include <vulkan/vulkan.hpp>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_raii.hpp>

namespace wind::vulkan {
inline VKAPI_ATTR auto VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT           message_severity,
                                                 [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT message_type,
                                                 const VkDebugUtilsMessengerCallbackDataEXT*      p_callback_data,
                                                 [[maybe_unused]] void* p_user_data) -> VkBool32
{
  if((message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0)
    spdlog::error("[VK] {}", p_callback_data->pMessage);
  else if((message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0)
    spdlog::warn("[VK] {}", p_callback_data->pMessage);
  else if((message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) != 0)
    spdlog::info("[VK] {}", p_callback_data->pMessage);

  return VK_FALSE;
}

auto create_debug_utils(const vk::raii::Instance& instance) noexcept -> WindResult<vk::raii::DebugUtilsMessengerEXT>;

};  // namespace wind::vulkan
