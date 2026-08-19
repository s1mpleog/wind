#pragma once

#include "platform/window.hpp"
#include "utils/expected_util.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace wind::vulkan {
struct Context
{
  vk::raii::Context  raii_ctx;
  vk::raii::Instance instance{nullptr};
#ifdef WIND_VULKAN_VALIDATION
  vk::raii::DebugUtilsMessengerEXT messenger{nullptr};
#endif
  vk::raii::SurfaceKHR surface{nullptr};
};

auto init(const platform::Window& window) noexcept -> WindResult<Context>;

}  // namespace wind::vulkan
