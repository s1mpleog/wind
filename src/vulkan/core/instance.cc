#include "instance.hpp"
#include "error.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/core/configuration.hpp"
#include <vulkan/vulkan.hpp>
#include <algorithm>
#include <array>
#include <expected>
#include <ranges>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>
#include <spdlog/spdlog.h>

namespace wind::vulkan {
static auto query_instance_layer_support(std::string_view requested_layer) -> WindResult<void>
{
  auto layers = vk::enumerateInstanceLayerProperties();

  if(!layers.has_value())
    return std::unexpected(WindError::vulkan(ErrorCode::InternalError, layers.result));

  // any_of checks if atlease one element in given range statisfies e.g. layers need to have atleast one requested_layer
  auto layer_found = std::ranges::any_of(*layers, [requested_layer](const vk::LayerProperties& lp) -> bool {
    return std::string_view{lp.layerName} == requested_layer;
  });

  if(!layer_found)
    return std::unexpected(WindError::vulkan(ErrorCode::LayerNotSupported, vk::Result::eErrorLayerNotPresent));

  return {};
}

static auto query_instance_extension_support(std::string_view requested_extension) -> WindResult<void>
{
  auto extensions = vk::enumerateInstanceExtensionProperties();

  if(!extensions.has_value())
    return std::unexpected(WindError::vulkan(ErrorCode::InternalError, extensions.result));

  auto layer_found = std::ranges::any_of(*extensions, [requested_extension](const vk::ExtensionProperties& lp) -> bool {
    return std::string_view{lp.extensionName} == requested_extension;
  });

  if(!layer_found)
    return std::unexpected(WindError::vulkan(ErrorCode::ExtensionNotSupported, vk::Result::eErrorExtensionNotPresent));

  return {};
}

// this function takes the ownership of extensions
[[nodiscard]] auto create_instance(const Configuration& cfg, const vk::raii::Context& ctx, std::vector<const char*> extensions) noexcept
    -> WindResult<vk::raii::Instance>
{
  auto inst_version = vk::enumerateInstanceVersion();

  if(!inst_version.has_value())
    return std::unexpected(WindError::vulkan(ErrorCode::InternalError, inst_version.result));

  if(inst_version.value < to_vk(cfg.api_version))
    return std::unexpected(WindError::vulkan(ErrorCode::VulkanVersion14NotFound, vk::Result::eErrorIncompatibleDriver));

#ifdef WIND_LOG_ENABLE
  spdlog::info("Vulkan API: {}.{}", vk::versionMajor(inst_version.value), vk::versionMinor(inst_version.value));
#endif

  // note: it is safe use {cfg.app_name.data(), cfg.engine..} here since its a string literal so it will have null terminator
  vk::ApplicationInfo app_info{cfg.app_name.data(), VK_MAKE_VERSION(0, 1, 0), cfg.engine_name.data(),
                               VK_MAKE_VERSION(0, 1, 0), to_vk(cfg.api_version)};

  vk::InstanceCreateInfo inst_info{};
  inst_info.pApplicationInfo = &app_info;

#ifdef WIND_VULKAN_VALIDATION
  std::array layers{"VK_LAYER_KHRONOS_validation"};

  for(auto&& [_, layer] : std::views::enumerate(layers))
  {
    WIND_TRY_VOID(query_instance_layer_support(layer));
  }

  // check for debug util extension
  WIND_TRY_VOID(query_instance_extension_support(VK_EXT_DEBUG_UTILS_EXTENSION_NAME));

  extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  inst_info.enabledLayerCount   = layers.size();
  inst_info.ppEnabledLayerNames = layers.data();

  inst_info.enabledExtensionCount   = extensions.size();
  inst_info.ppEnabledExtensionNames = extensions.data();

#ifdef WIND_LOG_ENABLE
  spdlog::info("creating instance | layers: {} | extensions: {}", layers.size(), extensions.size());
  std::ranges::for_each(layers, [](const auto l) -> auto { spdlog::info("  layer:     {}", l); });
  std::ranges::for_each(extensions, [](const auto e) -> auto { spdlog::info("  extension: {}", e); });
#endif

  // don't enable validation layer or debug extensions
#else
  inst_info.enabledLayerCount       = 0;
  inst_info.ppEnabledLayerNames     = nullptr;
  inst_info.enabledExtensionCount   = extensions.size();
  inst_info.ppEnabledExtensionNames = extensions.data();
#endif

  auto inst = ctx.createInstance(inst_info);

  if(!inst.has_value())
    return std::unexpected(WindError::vulkan(ErrorCode::FailedToCreateInstance, inst.result));

#ifdef WIND_LOG_ENABLE
  spdlog::info("Instance created successfully");
#endif

  return std::move(inst).value;
}

}  // namespace wind::vulkan
