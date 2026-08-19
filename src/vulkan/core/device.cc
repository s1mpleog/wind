#include "device.hpp"
#include "error.hpp"
#include "spdlog/spdlog.h"
#include "utils/expected_util.hpp"
#include "vulkan/core/configuration.hpp"
#include "vulkan/vulkan.hpp"
#include <algorithm>
#include <expected>
#include <optional>
#include <ranges>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace wind::vulkan::device {

struct PhysicalDeviceCandidate
{
  u32                          score{};
  vk::raii::PhysicalDevice     device{nullptr};
  vk::PhysicalDeviceProperties device_props{};
  std::optional<u32>           graphics_queue_index;
  std::optional<u32>           present_queue_index;
  std::optional<u32>           transfer_queue_index;
};

static auto make_physical_device_candidate(const vk::raii::PhysicalDevice& physical_device, const vk::raii::SurfaceKHR& surface)
    -> PhysicalDeviceCandidate
{
  std::optional<PhysicalDeviceCandidate> best{};

  auto features_2   = physical_device.getFeatures2().features;
  auto properties_2 = physical_device.getProperties2().properties;
  auto queue_infos  = physical_device.getQueueFamilyProperties();

  auto find_queue = [&](vk::QueueFlagBits flag) {
    auto it = std::ranges::find_if(queue_infos, [&](const vk::QueueFamilyProperties& queue_info) {
      return static_cast<bool>(queue_info.queueFlags & flag);
    });
    return it != queue_infos.end() ? std::optional{static_cast<u32>(std::distance(queue_infos.begin(), it))} : std::nullopt;
  };

  auto find_present_queue = [&](vk::SurfaceKHR surface) -> std::optional<u32> {
    for(u32 i = 0; i < queue_infos.size(); i++)
    {
      if(physical_device.getSurfaceSupportKHR(i, surface).has_value())
        return i;
    }
    return std::nullopt;
  };

  auto graphics_queue = find_queue(vk::QueueFlagBits::eGraphics);

  if(!graphics_queue)
    return {};

  auto present_queue = find_present_queue(surface);

  if(!present_queue)
    return {};

  PhysicalDeviceCandidate candiate{
      .score                = 0,
      .device_props         = properties_2,
      .graphics_queue_index = graphics_queue,
      .present_queue_index  = present_queue,
      .transfer_queue_index = {},
  };

  // i need transfer queue later

  auto transfer_queue = find_queue(vk::QueueFlagBits::eTransfer);

  if(transfer_queue)
  {
    candiate.score += 100;
    candiate.transfer_queue_index = transfer_queue;
  }

  if(features_2.samplerAnisotropy == vk::True)
    candiate.score += 100;

  if(properties_2.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
    candiate.score += 500;

  if(properties_2.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
    candiate.score += 300;

  if(properties_2.deviceType == vk::PhysicalDeviceType::eCpu)
    candiate.score += 150;

  if(!best || candiate.score > best->score)
  {
    best = candiate;
  }

  return best.value();
}

// responsible for selecting the preferred physical device
auto select_physical_device(const Configuration& cfg, const vk::raii::Instance& instance, const vk::raii::SurfaceKHR& surface) noexcept
    -> WindResult<void>
{
  auto physical_devices = instance.enumeratePhysicalDevices();

  if(!physical_devices.has_value())
    return std::unexpected(WindError::vulkan(ErrorCode::InternalError, physical_devices.result));

  if(physical_devices->empty())
    return std::unexpected(WindError::vulkan(ErrorCode::NoSuitablePhysicalDevice, vk::Result::eErrorUnknown));

  auto usable_devices = physical_devices.value | std::views::filter([&](const vk::raii::PhysicalDevice& device) -> bool {
                          auto features_2   = device.getFeatures2().features;
                          auto properties_2 = device.getProperties2().properties;
                          return features_2.geometryShader && properties_2.apiVersion >= to_vk(cfg.api_version);
                        });

  if(usable_devices.empty())
    return std::unexpected(WindError::vulkan(ErrorCode::NoSuitablePhysicalDevice, vk::Result::eErrorUnknown));

  auto candidates = usable_devices
                    | std::views::transform([&](const auto& dev) { return make_physical_device_candidate(dev, surface); });

  if(candidates.empty())
  {
    return std::unexpected(WindError::vulkan(ErrorCode::NoSuitablePhysicalDevice, vk::Result::eErrorUnknown));
  }

  auto best =
      std::ranges::max_element(candidates, [](const auto& lhs, const auto& rhs) -> auto { return lhs.score < rhs.score; });

  const auto& selected_gpu = *best;

#ifdef WIND_LOG_ENABLE
  spdlog::info("best device score is: {}, using GPU: {}, graphics_queue: {}, presentation_queue: {}, transfer_queue: {}",
               selected_gpu.score, std::string_view{selected_gpu.device_props.deviceName},
               selected_gpu.graphics_queue_index.value(), selected_gpu.present_queue_index.value(),
               selected_gpu.transfer_queue_index.value());
#endif

  return {};
};

[[nodiscard]] auto init(const Configuration& cfg, const vk::raii::Instance& instance, const vk::raii::SurfaceKHR& surface) noexcept
    -> WindResult<vk::raii::Device>
{

  WIND_TRY_VOID(select_physical_device(cfg, instance, surface));

  return nullptr;
}


};  // namespace wind::vulkan::device
