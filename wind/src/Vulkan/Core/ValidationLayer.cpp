#include "Vulkan/Core/ValidationLayer.hpp"
#include "Error.hpp"
#include <vulkan/vulkan_core.h>
#include "Vulkan/Types.hpp"

auto create_debug_utils(const Configuration& cfg, const vk::raii::Instance& instance) WIND_NOEXCEPT
    -> WindResult<vk::raii::DebugUtilsMessengerEXT>
{
  vk::DebugUtilsMessengerCreateInfoEXT create_info{};

  create_info.messageType     = to_vk(cfg.debug_message_type);
  create_info.messageSeverity = to_vk(cfg.debug_message_severity);
  create_info.pfnUserCallback = debug_callback;

  auto messenger = WIND_TRY(instance.createDebugUtilsMessengerEXT(create_info), ErrorCode::FailedToCreateDebugMessenger);

  return messenger;
}

