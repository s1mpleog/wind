#pragma once

#include "platform/window.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/core/configuration.hpp"
#include "vulkan/core/device.hpp"
#include "vulkan/core/swapchain.hpp"

#include <vulkan/vulkan_core.h>

namespace wind::vulkan {

constexpr usize MAX_FRAME_IN_FLIGHT = 2;

struct VulkanContext
{
  vk::raii::Context  raii_ctx;
  vk::raii::Instance instance{nullptr};
#ifdef WIND_VULKAN_VALIDATION
  vk::raii::DebugUtilsMessengerEXT messenger{nullptr};
#endif
  vk::raii::SurfaceKHR surface{nullptr};
  GpuDevice            gpu_device{};
  SwapchainContext     swapchain{};
};

auto create_context(const platform::Window& window, const Configuration& cfg) WIND_NOEXCEPT -> WindResult<VulkanContext>;

}  // namespace wind::vulkan
