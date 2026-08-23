#include "device.hpp"
#include "error.hpp"
#include "spdlog/spdlog.h"
#include "utils/expected_util.hpp"
#include "vulkan/core/configuration.hpp"
#include "vulkan/vulkan.hpp"
#include <algorithm>
#include <optional>
#include <ranges>
#include <set>
#include <string_view>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>
#include "vulkan/types.hpp"

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

  auto transfer_queue = find_queue(vk::QueueFlagBits::eTransfer);

  PhysicalDeviceCandidate candidate{
      .score                = 0,
      .device               = physical_device,
      .device_props         = properties_2,
      .graphics_queue_index = graphics_queue,
      .present_queue_index  = present_queue,
      .transfer_queue_index = {},
  };

  if(transfer_queue)
  {
    candidate.score += 100;
    candidate.transfer_queue_index = transfer_queue;
  }

  if(features_2.samplerAnisotropy == vk::True)
    candidate.score += 100;

  if(properties_2.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
    candidate.score += 500;

  if(properties_2.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
    candidate.score += 300;

  if(properties_2.deviceType == vk::PhysicalDeviceType::eCpu)
    candidate.score += 150;

  if(!best || candidate.score > best->score)
  {
    best = candidate;
  }

  return best.value();
}

WIND_NODISCARD auto check_hard_requirements(const Configuration& cfg, const vk::raii::PhysicalDevice& device) WIND_NOEXCEPT -> bool
{
  auto features_2   = device.getFeatures2().features;
  auto properties_2 = device.getProperties2().properties;

  auto chain =
      device.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceSwapchainMaintenance1FeaturesKHR>();

  auto& features_13 = chain.get<vk::PhysicalDeviceVulkan13Features>();

  auto& maintainance_feature = chain.get<vk::PhysicalDeviceSwapchainMaintenance1FeaturesKHR>();

  auto available_extensions = device.enumerateDeviceExtensionProperties();

  // limitation can't use WindResult :(
  if(!available_extensions || available_extensions->empty())
    return false;

  const std::array<std::string_view, 3> required_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
                                                               VK_KHR_MAINTENANCE1_EXTENSION_NAME};

  // for all required extensions:
  // does any of available extensions matches the requirement
  // if available extensions = {"A", "B", "C"}
  // and required extensions = {"A", "B"}
  // then
  // "A" == "A" -> any_of true
  // "B" == "B" -> any_of true
  const auto supported = std::ranges::all_of(required_extensions, [&](std::string_view required) {
    return std::ranges::any_of(*available_extensions, [&](const vk::ExtensionProperties& extension) {
      return required == extension.extensionName;
    });
  });

  return (features_2.geometryShader == vk::True) && properties_2.apiVersion >= to_vk(cfg.api_version)
         && (features_13.dynamicRendering == vk::True) && (features_13.synchronization2 == vk::True)
         && (maintainance_feature.swapchainMaintenance1 == vk::True) && supported;
}

// responsible for selecting the preferred physical device
WIND_NODISCARD static auto select_physical_device(const Configuration&      cfg,
                                                  const vk::raii::Instance& instance,
                                                  const vk::raii::SurfaceKHR& surface) WIND_NOEXCEPT -> WindResult<PhysicalDeviceCandidate>
{
  auto physical_devices = WIND_TRY(instance.enumeratePhysicalDevices(), ErrorCode::InternalError);

  WIND_ENSURE_NOT_EMPTY(physical_devices, WindError::vulkan(ErrorCode::NoSuitablePhysicalDevice));

  auto usable_devices = physical_devices | std::views::filter([&](const vk::raii::PhysicalDevice& device) -> bool {
                          return check_hard_requirements(cfg, device);
                        });

  WIND_ENSURE_NOT_EMPTY(usable_devices, WindError::vulkan(ErrorCode::NoSuitablePhysicalDevice));

  auto candidates = usable_devices
                    | std::views::transform([&](const auto& dev) { return make_physical_device_candidate(dev, surface); });

  WIND_ENSURE_NOT_EMPTY(candidates, WindError::vulkan(ErrorCode::NoSuitablePhysicalDevice));

  auto best =
      std::ranges::max_element(candidates, [](const auto& lhs, const auto& rhs) -> auto { return lhs.score < rhs.score; });

  // we already checked candidates empty so at this point we are sure that there will be atleast one candidate
  auto selected_gpu = *best;

#ifdef WIND_LOG_ENABLE
  spdlog::info("using GPU: {}, graphics_queue: {}, presentation_queue: {}, transfer_queue: {}",
               std::string_view{selected_gpu.device_props.deviceName}, selected_gpu.graphics_queue_index.value(),
               selected_gpu.present_queue_index.value(), selected_gpu.transfer_queue_index.value());
#endif

  return selected_gpu;
};

WIND_NODISCARD static auto create_logical_device(PhysicalDeviceCandidate candidate) WIND_NOEXCEPT -> WindResult<DeviceContext>
{
  // candidate already have all the hard requirements check just create device here
  vk::PhysicalDeviceVulkan13Features features_13{};
  features_13.dynamicRendering = vk::True;
  features_13.synchronization2 = vk::True;

  vk::PhysicalDeviceSwapchainMaintenance1FeaturesEXT maintenance_feature{};
  maintenance_feature.swapchainMaintenance1 = vk::True;

  features_13.pNext = &maintenance_feature;

  float queue_priorities = 1.0F;

  std::set unique_queues{candidate.graphics_queue_index.value(), candidate.present_queue_index.value()};

  if(candidate.present_queue_index)
    unique_queues.insert(candidate.present_queue_index.value());

  std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
  queue_create_infos.reserve(unique_queues.size());

  for(auto idx : unique_queues)
  {
    queue_create_infos.emplace_back(vk::DeviceQueueCreateFlags{}, idx, 1, &queue_priorities);
  }

#ifdef WIND_LOG_ENABLE
  spdlog::info("using {} queues", queue_create_infos.size());
#endif

  // NOTE: i am using this same array in two places i should put this somewhere maybe some DeviceConfiguration or something
  const std::array<const char*, 3> enabled_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
                                                         VK_KHR_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME};

  vk::DeviceCreateInfo create_info{};
  create_info.pNext                   = &features_13;
  create_info.enabledExtensionCount   = enabled_extensions.size();
  create_info.ppEnabledExtensionNames = enabled_extensions.data();
  create_info.queueCreateInfoCount    = queue_create_infos.size();
  create_info.pQueueCreateInfos       = queue_create_infos.data();

  WIND_ASSERT(candidate.device != nullptr && "Physical device is null");

  auto device = WIND_TRY(candidate.device.createDevice(create_info), ErrorCode::FailedToCreateDevice);

#ifdef WIND_LOG_ENABLE
  spdlog::info("Creating logical device, enabled extensions: {}", enabled_extensions.size());
#endif

  DeviceContext context{};

  context.handle                 = std::move(device);
  context.physical_device        = std::move(candidate.device);
  context.physical_device_props  = candidate.device_props;
  context.graphics_queue_idx     = candidate.graphics_queue_index;
  context.presentation_queue_idx = candidate.present_queue_index;
  context.graphics_queue         = context.handle.getQueue(context.graphics_queue_idx.value(), 0);
  context.presentation_queue     = context.handle.getQueue(context.presentation_queue_idx.value(), 0);

  if(candidate.transfer_queue_index) [[likely]]
  {
    context.transfer_queue_idx = candidate.transfer_queue_index;
    context.presentation_queue = context.handle.getQueue(context.transfer_queue_idx.value(), 0);
  }

  return context;
};

WIND_NODISCARD static auto create_command_pool(const vk::raii::Device& device, u32 queue_index) WIND_NOEXCEPT
    -> WindResult<vk::raii::CommandPool>
{
  vk::CommandPoolCreateInfo cmd_pool_create_info{};
  cmd_pool_create_info.queueFamilyIndex = queue_index;
  cmd_pool_create_info.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

  return WIND_TRY(device.createCommandPool(cmd_pool_create_info));
}

WIND_NODISCARD auto create(const Configuration& cfg, const vk::raii::Instance& instance, const vk::raii::SurfaceKHR& surface) WIND_NOEXCEPT
    -> WindResult<DeviceContext>
{
  auto candidate      = WIND_TRY(select_physical_device(cfg, instance, surface));
  auto device_context = WIND_TRY(create_logical_device(std::move(candidate)));

  device_context.graphics_pool =
      WIND_TRY(create_command_pool(device_context.handle, device_context.graphics_queue_idx.value()));

  if(device_context.has_transfer_queue()
     && device_context.transfer_queue_idx.value() != device_context.graphics_queue_idx.value())
  {
    device_context.transfer_pool =
        WIND_TRY(create_command_pool(device_context.handle, device_context.transfer_queue_idx.value()));
  }

  return device_context;
}

};  // namespace wind::vulkan::device
