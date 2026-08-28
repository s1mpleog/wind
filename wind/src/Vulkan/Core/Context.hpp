#pragma once

#include "Platform/Window.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Configuration.hpp"
#include "Vulkan/Core/Device.hpp"
#include <vulkan/vulkan_core.h>

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

auto create_context(const Window& window, const Configuration& cfg) WIND_NOEXCEPT -> WindResult<VulkanContext>;
