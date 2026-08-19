#include "validation_layer.hpp"
#include "error.hpp"
#include "vulkan/core/configuration.hpp"
#include <vulkan/vulkan_core.h>

namespace wind::vulkan {
auto create_debug_utils(const Configuration& cfg, const vk::raii::Instance& instance) noexcept
    -> WindResult<vk::raii::DebugUtilsMessengerEXT>
{
  vk::DebugUtilsMessengerCreateInfoEXT create_info{};

  create_info.messageType     = to_vk(cfg.debug_message_type);
  create_info.messageSeverity = to_vk(cfg.debug_message_severity);
  create_info.pfnUserCallback = debug_callback;

  auto messenger = instance.createDebugUtilsMessengerEXT(create_info);

  if(!messenger.has_value())
    return std::unexpected(WindError::vulkan(ErrorCode::FailedToCreateDebugMessenger, messenger.result));

  return std::move(messenger).value;
}

}  // namespace wind::vulkan
