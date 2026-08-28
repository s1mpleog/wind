#pragma once

#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Configuration.hpp"

#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

WIND_INLINE auto DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
                               [[maybe_unused]] vk::DebugUtilsMessageTypeFlagsEXT MessageType,
                               const vk::DebugUtilsMessengerCallbackDataEXT *PCallbackData,
                               [[maybe_unused]] void *PUserData) -> vk::Bool32
{
	if (MessageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
		spdlog::error("[VK] {}", PCallbackData->pMessage);
	else if (MessageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
		spdlog::warn("[VK] {}", PCallbackData->pMessage);
	else if (MessageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo)
		spdlog::info("[VK] {}", PCallbackData->pMessage);

	return VK_FALSE;
}

auto CreateDebugUtils(const FConfiguration &Cfg, const vk::raii::Instance &Instance) WIND_NOEXCEPT
    -> WindResult<vk::raii::DebugUtilsMessengerEXT>;
