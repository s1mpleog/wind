#include "Vulkan/Core/ValidationLayer.hpp"

#include "Error.hpp"
#include "Vulkan/Types.hpp"

#include <vulkan/vulkan_core.h>

auto CreateDebugUtils(const FConfiguration &Cfg, const vk::raii::Instance &Instance) WIND_NOEXCEPT
    -> TWindResult<vk::raii::DebugUtilsMessengerEXT>
{
	vk::DebugUtilsMessengerCreateInfoEXT CreateInfo{};

	CreateInfo.messageType = ToVk(Cfg.DebugMessageType);
	CreateInfo.messageSeverity = ToVk(Cfg.DebugMessageSeverity);
	CreateInfo.pfnUserCallback = DebugCallback;

	auto Messenger =
	    WIND_TRY(Instance.createDebugUtilsMessengerEXT(CreateInfo), ErrorCode::FailedToCreateDebugMessenger);

	return Messenger;
}
