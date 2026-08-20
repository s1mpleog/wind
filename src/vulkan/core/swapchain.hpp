#pragma once

#include "utils/expected_util.hpp"
#include "vulkan/core/configuration.hpp"
#include "vulkan/core/device.hpp"
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
                           const DeviceContext&        device_context) WIND_NOEXCEPT -> WindResult<SwapchainContext>;
};

};  // namespace wind::vulkan
