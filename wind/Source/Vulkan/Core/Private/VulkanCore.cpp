#include "VulkanCore.hpp"

#include "Core/Public/Check.hpp"
#include "Vulkan/Core/Private/VulkanDevice.h"
#include "Vulkan/Core/Private/VulkanExtension.hpp"
#include "Vulkan/Core/Private/VulkanGenericPlatform.h"
#include "Vulkan/Types.hpp"
#include "spdlog/spdlog.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_raii.hpp"

#include <algorithm>
#include <memory>
#include <ranges>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_to_string.hpp>

static vk::raii::PhysicalDevice SelectPhysicalDevice(const vk::raii::Instance &Instance)
{
	VERIFYVULKANRESULT_UNWRAP(PhysicalDevices, Instance.enumeratePhysicalDevices());

	CHECK(PhysicalDevices.size() >= 1, "SelectPhysicalDevice could not find a compatible Vulkan device or driver "
	                                   "(EnumeratePhysicalDevices returned 0 devices).  "
	                                   "Make sure your video card supports Vulkan and try updating your video driver "
	                                   "to a more recent version (proceed with any pending reboots).");

	// for now just return discrete gpu if available or fallback to integrated one later when i have
	// command line and GUI then come back here and add features to toggle device from gpu or read cli value

	struct FPhysicalDeviceInfo
	{
		FPhysicalDeviceInfo() = delete;

		FPhysicalDeviceInfo(uint32 OriginalIndex, vk::raii::PhysicalDevice InPhysicalDevice)
		    : OriginalIndex(OriginalIndex), PhysicalDevice(std::move(InPhysicalDevice))
		{
			PhysicalDeviceProperties2.pNext = &PhysicalDeviceIdProperties;
			PhysicalDevice.getProperties2(&PhysicalDeviceProperties2);
		};

		uint32 OriginalIndex{};
		vk::raii::PhysicalDevice PhysicalDevice{VK_NULL_HANDLE};
		vk::PhysicalDeviceProperties2 PhysicalDeviceProperties2{};
		vk::PhysicalDeviceIDProperties PhysicalDeviceIdProperties{};
	};

	std::vector<FPhysicalDeviceInfo> PhysicalDeviceInfos;
	PhysicalDeviceInfos.reserve(PhysicalDevices.size());

	for (auto &&[Index, PhysicalDevice] : std::views::enumerate(PhysicalDevices))
	{
		// only push if device api version is >= 1.3
		vk::PhysicalDeviceProperties PhysicalDeviceProperties = PhysicalDevice.getProperties();

		if (PhysicalDeviceProperties.apiVersion < vk::ApiVersion13)
		{
			continue;
		}

		PhysicalDeviceInfos.emplace_back(Index, std::move(PhysicalDevice));
	}

	std::ranges::sort(PhysicalDeviceInfos,
	                  [](const FPhysicalDeviceInfo &Lhs, const FPhysicalDeviceInfo &Rhs)
	                  {
		                  if (Lhs.PhysicalDeviceProperties2.properties.deviceType ==
		                      Rhs.PhysicalDeviceProperties2.properties.deviceType)
		                  {
			                  return Lhs.OriginalIndex < Rhs.OriginalIndex;
		                  }

		                  // prefer Discrete gpu first
		                  return (Lhs.PhysicalDeviceProperties2.properties.deviceType ==
		                          vk::PhysicalDeviceType::eDiscreteGpu) ||
		                         (Rhs.PhysicalDeviceProperties2.properties.deviceType == vk::PhysicalDeviceType::eCpu);
	                  });

	CHECK(!PhysicalDeviceInfos.empty(), "Failed to find any valid suitable Device for Engine...");

	return PhysicalDeviceInfos[0].PhysicalDevice;
}

FVulkanCore::FVulkanCore(FConfiguration &InConfig) : Instance(VK_NULL_HANDLE), Device(VK_NULL_HANDLE), Config(InConfig)
{
	// TODO: do other vulkan related stuffs

	CreateInstance();
	SelectDevice();
}

void FVulkanCore::CreateInstance()
{
	VERIFYVULKANRESULT_UNWRAP(VulkanApiVersion, vk::enumerateInstanceVersion());
	CHECK(VulkanApiVersion >= vk::ApiVersion13,
	      "Engine needs at least vulkan version 1.3 or later to run try updating the GPU driver");

	// FIXME: i have to handle WSI extensions somehow

	FVulkanInstanceExtensionArray WindInstanceExtensions =
	    FVulkanInstanceExtension::GetWindSupportedInstanceExtensions(ToVk(Config.ApiVersion));

	for (std::unique_ptr<FVulkanInstanceExtension> &Extension : WindInstanceExtensions)
	{
		if (Extension->InUse())
		{
			InstanceExtensions.emplace_back(Extension->GetExtensionName());
		}
	}

	vk::ApplicationInfo AppInfo{};
	AppInfo.applicationVersion = vk::makeVersion(0, 1, 0);
	AppInfo.pApplicationName = "Wind";
	AppInfo.pEngineName = "Wind Engine";
	AppInfo.apiVersion = ToVk(Config.ApiVersion);

	vk::InstanceCreateInfo InstInfo{};

	InstInfo.pApplicationInfo = &AppInfo;
	InstInfo.enabledExtensionCount = InstanceExtensions.size();
	InstInfo.ppEnabledExtensionNames = InstanceExtensions.data();

	// TODO: move this into Extension system later
	const char *LayerName = "VK_LAYER_KHRONOS_validation";

#ifdef WIND_VULKAN_VALIDATION
	InstInfo.enabledLayerCount = 1;
	InstInfo.ppEnabledLayerNames = &LayerName;
#elif
	InstInfo.enabledLayerCount = 0;
	InstInfo.ppEnabledLayerNames = nullptr;
#endif

	vk::ResultValueType<vk::raii::Instance>::type InstanceResult = Context.createInstance(InstInfo);

	if (!InstanceResult && InstanceResult.error() == vk::Result::eErrorIncompatibleDriver)
	{
		FATAL("Cannot find a compatible Vulkan driver (ICD).\n\nPlease look at the Getting Started guide for "
		      "additional information.");
	}

	if (!InstanceResult && InstanceResult.error() == vk::Result::eErrorExtensionNotPresent)
	{
		const char *MissingExtensionName = nullptr;
		for (const char *Extension : InstanceExtensions)
		{
			spdlog::error("Missing required Vulkan extensions: %s", Extension);
			MissingExtensionName = Extension;
		}

		FATAL(
		    "during instance creation vulkan did not find requested instance extension: {} make sure vulkan is updated "
		    "to latest version prefer 1.4 over 1.3 if your GPU supports it",
		    MissingExtensionName);
	}

	if (!InstanceResult && InstanceResult.error() == vk::Result::eErrorLayerNotPresent)
	{
		FATAL("Note the `VK_LAYER_KHRONOS_validation` was requested during Instance creation but vulkan did not found "
		      "it try disabling `WIND_VULKAN_VALIDATION` if you don't want validation layer debugging");
	}

	CHECK(InstanceResult.has_value(),
	      "Failed to create the vulkan instance this could only happen if something has really gone wrong try updating "
	      "the graphics driver or reboot we don't know what happen really you should see vulkan error code in crash "
	      "file try searching for more information");

	Instance = std::move(InstanceResult.value());

	WIND_LOG(info, "Instance created successfully");

#if WIND_VULKAN_VALIDATION
	SetupDebugCallbacks();
	WIND_LOG(info, "Debug utils messenger setup success");
#endif
}

void FVulkanCore::SelectDevice()
{
	vk::raii::PhysicalDevice PhysicalDevice = SelectPhysicalDevice(Instance);
	Device = std::make_unique<FVulkanDevice>(PhysicalDevice);
}

void FVulkanCore::Initialize() WIND_NOEXCEPT
{
	Device->InitGpu(Context);
}
