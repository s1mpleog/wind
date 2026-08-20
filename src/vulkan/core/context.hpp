#pragma once

#include "platform/window.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/core/configuration.hpp"
#include "vulkan/core/device.hpp"
#include "vulkan/core/swapchain.hpp"

#include <vulkan/vulkan_core.h>

namespace wind::vulkan {
struct Context
{
  vk::raii::Context  raii_ctx;
  vk::raii::Instance instance{nullptr};
#ifdef WIND_VULKAN_VALIDATION
  vk::raii::DebugUtilsMessengerEXT messenger{nullptr};
#endif
  vk::raii::SurfaceKHR surface{nullptr};
  DeviceContext        device_ctx{};
  SwapchainContext     swapchain_ctx{};
};

auto init(const platform::Window& window, Configuration cfg) WIND_NOEXCEPT -> WindResult<Context>;

}  // namespace wind::vulkan
