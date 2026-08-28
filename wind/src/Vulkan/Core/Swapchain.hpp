#pragma once

#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Configuration.hpp"
#include "Vulkan/Core/Device.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>


struct SwapchainContext
{
  vk::raii::SwapchainKHR           handle{nullptr};
  std::vector<vk::Image>           images;
  std::vector<vk::raii::ImageView> image_views;
  vk::SurfaceFormatKHR             format{};
  vk::Extent2D                     extent{};
};

WIND_NODISCARD auto create_swapchain(const Configuration&          cfg,
                                     u32                           window_width,
                                     u32                           window_height,
                                     const vk::raii::SurfaceKHR&   surface,
                                     const GpuDevice&              device_context,
                                     const vk::raii::SwapchainKHR* old_swapchain = nullptr) WIND_NOEXCEPT
    -> WindResult<SwapchainContext>;
