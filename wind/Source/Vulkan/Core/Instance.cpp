#include "Vulkan/Core/Instance.hpp"

#include "Check.hpp"
#include "Error.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Configuration.hpp"
#include "Vulkan/Core/Private/VulkanExtension.hpp"
#include "vulkan/vulkan.hpp"

#include <Vulkan/Types.hpp>
#include <algorithm>
#include <array>
#include <ranges>
#include <spdlog/spdlog.h>
#include <string_view>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

static auto QueryInstanceLayerSupport(std::string_view RequestedLayer) -> TWindResult<void>
{
	auto Layers = WIND_TRY(vk::enumerateInstanceLayerProperties());

	// any_of checks if atlease one element in given range satisfies e.g. layers need to have at least one
	// requested_layer
	auto LayerFound = std::ranges::any_of(Layers, [RequestedLayer](const vk::LayerProperties &Lp) -> bool
	                                      { return std::string_view{Lp.layerName} == RequestedLayer; });

	if (!LayerFound)
		WIND_ERR(WindError::vulkan(ErrorCode::LayerNotSupported, vk::Result::eErrorLayerNotPresent));

	return {};
}

static auto QueryInstanceExtensionSupport(std::string_view RequestedExtension) -> TWindResult<void>
{
	auto Extensions = WIND_TRY(vk::enumerateInstanceExtensionProperties());

	auto LayerFound = std::ranges::any_of(Extensions, [RequestedExtension](const vk::ExtensionProperties &Lp) -> bool
	                                      { return std::string_view{Lp.extensionName} == RequestedExtension; });

	if (!LayerFound)
		WIND_ERR(WindError::vulkan(ErrorCode::ExtensionNotSupported, vk::Result::eErrorExtensionNotPresent));

	return {};
}

// this function takes the ownership of extensions
WIND_NODISCARD auto Create(const FConfiguration &Cfg, const vk::raii::Context &Ctx,
                           std::vector<const char *> Extensions) WIND_NOEXCEPT -> TWindResult<vk::raii::Instance>
{
	std::vector<vk::ExtensionProperties> DriverInstanceExtensions =
	    FVulkanInstanceExtension::GetDriverSupportedInstanceExtensions();

	CHECK(FVulkanInstanceExtension::FindExtension(DriverInstanceExtensions, "VK_KHR_surface"),
	      "Engine needs Surface extension to display current Vulkan Loader does not supports it");

	auto InstVersion = WIND_TRY(vk::enumerateInstanceVersion());

	if (InstVersion < ToVk(Cfg.ApiVersion))
		WIND_ERR(WindError::vulkan(ErrorCode::VulkanVersion14NotFound, vk::Result::eErrorIncompatibleDriver));

#ifdef WIND_LOG_ENABLE
	spdlog::info("Vulkan API: {}.{}", vk::versionMajor(InstVersion), vk::versionMinor(InstVersion));
#endif

	// note: it is safe use {cfg.app_name.data(), cfg.engine..} here since its a string literal so it will have null
	// terminator
	vk::ApplicationInfo AppInfo{Cfg.AppName.data(), VK_MAKE_VERSION(0, 1, 0), Cfg.EngineName.data(),
	                            VK_MAKE_VERSION(0, 1, 0), ToVk(Cfg.ApiVersion)};

	vk::InstanceCreateInfo InstInfo{};
	InstInfo.pApplicationInfo = &AppInfo;

	WIND_TRY_VOID(QueryInstanceExtensionSupport(VK_KHR_SURFACE_MAINTENANCE_1_EXTENSION_NAME));
	WIND_TRY_VOID(QueryInstanceExtensionSupport(VK_KHR_SURFACE_EXTENSION_NAME));
	WIND_TRY_VOID(QueryInstanceExtensionSupport(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME));

	Extensions.emplace_back(VK_KHR_SURFACE_MAINTENANCE_1_EXTENSION_NAME);
	Extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);
	Extensions.emplace_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);

#ifdef WIND_VULKAN_VALIDATION
	std::array Layers{"VK_LAYER_KHRONOS_validation"};

	for (auto &&[_, layer] : std::views::enumerate(Layers))
	{
		WIND_TRY_VOID(QueryInstanceLayerSupport(layer));
	}

	// check for debug util extension
	WIND_TRY_VOID(QueryInstanceExtensionSupport(VK_EXT_DEBUG_UTILS_EXTENSION_NAME));

	Extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	InstInfo.enabledLayerCount = Layers.size();
	InstInfo.ppEnabledLayerNames = Layers.data();

	InstInfo.enabledExtensionCount = Extensions.size();
	InstInfo.ppEnabledExtensionNames = Extensions.data();

#ifdef WIND_LOG_ENABLE
	spdlog::info("creating instance | layers: {} | extensions: {}", Layers.size(), Extensions.size());
	std::ranges::for_each(Layers, [](const auto L) -> auto { spdlog::info("  layer:     {}", L); });
	std::ranges::for_each(Extensions, [](const auto E) -> auto { spdlog::info("  extension: {}", E); });
#endif

	// don't enable validation layer or debug extensions
#else
	inst_info.enabledLayerCount = 0;
	inst_info.ppEnabledLayerNames = nullptr;
	inst_info.enabledExtensionCount = extensions.size();
	inst_info.ppEnabledExtensionNames = extensions.data();
#endif

	auto Inst = WIND_TRY(Ctx.createInstance(InstInfo), ErrorCode::FailedToCreateInstance);

#ifdef WIND_LOG_ENABLE
	spdlog::info("Instance created successfully");
#endif

	return Inst;
}
