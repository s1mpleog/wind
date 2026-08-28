#pragma once

#include "Types.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Configuration.hpp"
#include <vulkan/vulkan.hpp>
#include <optional>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

struct GpuDevice
{
  vk::raii::PhysicalDevice             physical_device{nullptr};
  vk::raii::Device                     device{nullptr};
  vk::raii::Queue                      graphics_queue{nullptr};
  vk::raii::Queue                      presentation_queue{nullptr};
  std::optional<vk::raii::Queue>       transfer_queue;
  std::optional<u32>                   graphics_queue_idx;
  std::optional<u32>                   presentation_queue_idx;
  std::optional<u32>                   transfer_queue_idx;
  vk::PhysicalDeviceProperties         physical_device_props{};
  vk::raii::CommandPool                graphics_pool{nullptr};
  std::optional<vk::raii::CommandPool> transfer_pool;

  constexpr auto has_transfer_queue() const WIND_NOEXCEPT -> bool
  {
    return transfer_queue_idx.has_value() && transfer_queue.has_value();
  }
};


WIND_NODISCARD auto device_create(const Configuration& cfg, const vk::raii::Instance& instance, const vk::raii::SurfaceKHR& surface) WIND_NOEXCEPT
    -> WindResult<GpuDevice>;
