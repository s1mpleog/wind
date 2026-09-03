#include "Core/Public/Check.hpp"
#include "Vulkan/Types.hpp"
#include "VulkanCore.hpp"
#include "vulkan/vulkan_raii.hpp"

#include <algorithm>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

static VKAPI_ATTR vk::Bool32 VKAPI_CALL
DebugUtilsCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT MsgSeverity,
                   [[maybe_unused]] vk::DebugUtilsMessageTypeFlagsEXT MessageType,
                   const vk::DebugUtilsMessengerCallbackDataEXT *CallbackData, [[maybe_unused]] void *UserData)
{
	if (MsgSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
		spdlog::error("[VK] {}", CallbackData->pMessage);
	else if (MsgSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
		spdlog::warn("[VK] {}", CallbackData->pMessage);
	else if (MsgSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo)
		spdlog::info("[VK] {}", CallbackData->pMessage);

	return vk::False;
}

void FVulkanCore::SetupDebugCallbacks()
{
	if (std::ranges::find(InstanceExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != InstanceExtensions.end())
	{
		vk::DebugUtilsMessengerCreateInfoEXT CreateInfo{};

		CreateInfo.messageType = ToVk(Config.DebugMessageType);
		CreateInfo.messageSeverity = ToVk(Config.DebugMessageSeverity);
		CreateInfo.pfnUserCallback = DebugUtilsCallback;

		vk::ResultValueType<vk::DebugUtilsMessengerEXT>::type MessengerResult =
		    Instance.createDebugUtilsMessengerEXT(CreateInfo);

		CHECK(MessengerResult.has_value(), "NOTE: `WIND_VULKAN_VALIDATION` was enabled so engine tried to created "
		                                   "Debug utils messenger but vulkan failed to create it");

		Messenger = std::move(MessengerResult.value());
	}
}
