#pragma once

#include "platform/Window.hpp"
#include "utils/ExpectedUtil.hpp"
#include "vulkan/core/Configuration.hpp"
#include "vulkan/core/Device.hpp"
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
};

auto create_context(const platform::Window& window, const Configuration& cfg) WIND_NOEXCEPT -> WindResult<VulkanContext>;

}  // namespace wind::vulkan
