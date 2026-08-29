#pragma once

#include "Platform/Window.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Configuration.hpp"
#include "Vulkan/Core/VulkanDevice.hpp"

#include <vulkan/vulkan_core.h>

constexpr size_t MaxFrameInFlight = 2;

struct FVulkanContext
{
	vk::raii::Context RaiiCtx;
	vk::raii::Instance Instance{nullptr};
#ifdef WIND_VULKAN_VALIDATION
	vk::raii::DebugUtilsMessengerEXT Messenger{nullptr};
#endif
	vk::raii::SurfaceKHR Surface{nullptr};
	FGpuDevice GpuDevice{};
};

auto CreateContext(const FUWindow &Window, const FConfiguration &Cfg) WIND_NOEXCEPT -> TWindResult<FVulkanContext>;
