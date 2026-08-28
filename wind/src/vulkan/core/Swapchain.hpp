#pragma once

#include "utils/ExpectedUtil.hpp"
#include "vulkan/core/Configuration.hpp"
#include "vulkan/core/Device.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>

namespace wind::vulkan {

struct SwapchainContext
{
  vk::raii::SwapchainKHR           handle{nullptr};
  std::vector<vk::Image>           images;
  std::vector<vk::raii::ImageView> image_views;
  vk::SurfaceFormatKHR             format{};
  vk::Extent2D                     extent{};
};

namespace swapchain {
WIND_NODISCARD auto create(const Configuration&        cfg,
                           u32                         window_width,
                           u32                         window_height,
                           const vk::raii::SurfaceKHR& surface,
                           const GpuDevice&        device_context,
                           const vk::raii::SwapchainKHR* old_swapchain = nullptr) WIND_NOEXCEPT -> WindResult<SwapchainContext>;
};

};  // namespace wind::vulkan
