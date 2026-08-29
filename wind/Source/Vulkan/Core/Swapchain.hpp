#pragma once

#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Configuration.hpp"
#include "Vulkan/Core/VulkanDevice.hpp"

#include <vector>
#include <vulkan/vulkan_core.h>

struct FSwapchainContext
{
	vk::raii::SwapchainKHR Handle{nullptr};
	std::vector<vk::Image> Images;
	std::vector<vk::raii::ImageView> ImageViews;
	vk::SurfaceFormatKHR Format{};
	vk::Extent2D Extent{};
};

WIND_NODISCARD auto CreateSwapchain(const FConfiguration &Cfg, uint32 WindowWidth, uint32 WindowHeight,
                                    const vk::raii::SurfaceKHR &Surface, const FGpuDevice &DeviceContext,
                                    const vk::raii::SwapchainKHR *OldSwapchain = nullptr) WIND_NOEXCEPT
    -> TWindResult<FSwapchainContext>;
