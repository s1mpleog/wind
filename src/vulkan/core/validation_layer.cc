#include "validation_layer.hpp"
#include "error.hpp"

namespace wind::vulkan {
auto create_debug_utils(const vk::raii::Instance& instance) noexcept -> WindResult<vk::raii::DebugUtilsMessengerEXT>
{
  //TODO: later this will come from VulkanConfiguration
  VkDebugUtilsMessengerCreateInfoEXT create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

  create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
                                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
  ;
  create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  ;
  create_info.pfnUserCallback = debug_callback;

  auto messenger = instance.createDebugUtilsMessengerEXT(create_info);

  if(!messenger.has_value())
    return std::unexpected(WindError::vulkan(ErrorCode::FailedToCreateDebugMessenger, messenger.result));

  return std::move(*messenger);
}
}  // namespace wind::vulkan
