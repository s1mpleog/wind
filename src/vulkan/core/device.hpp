#pragma once

#include "types.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/core/configuration.hpp"
#include "vulkan/vulkan.hpp"
#include <optional>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

namespace wind::vulkan {

struct DeviceContext
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

namespace device {
WIND_NODISCARD auto create(const Configuration& cfg, const vk::raii::Instance& instance, const vk::raii::SurfaceKHR& surface) WIND_NOEXCEPT
    -> WindResult<DeviceContext>;
};

};  // namespace wind::vulkan
