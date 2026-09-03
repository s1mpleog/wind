#pragma once

#include "Config.hpp"
#include "Vulkan/Core/Private/Linux/VulkanLinux.hpp"
#include "Vulkan/Core/Private/VulkanGenericPlatform.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"

#include <algorithm>
#include <array>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_core.h>

struct FOptionalVulkanDeviceExtensions;

class FVulkanDevice;

// what i learn from unreal design
// there should be a base class if other classes share similar features
// for example here both instance and device extension class needs find and GetName right
// so {instance, device} derives ExtensionBase
// complexity should be hidden call site must not do much most of the work should
// happen internally
// forward deceleration is not a bad thing
// using unique ptr and all is not bad in initialization code
// don't be scared of writing long names
// ownership intent matters more then performance in initialization code that's like gonna run once
// forward decleration is ok to reduce compilation time

class FVulkanExtensionBase
{
  public:
	explicit FVulkanExtensionBase(const char *InExtensionName, bool bInIsEnabled)
	    : ExtensionName(InExtensionName), bIsEnabled(bInIsEnabled) {};

	virtual ~FVulkanExtensionBase() = default;

	static bool FindExtension(const std::span<vk::ExtensionProperties> WindExtensions,
	                          const std::string_view ExtensionName)
	{
		return std::ranges::any_of(WindExtensions, [&](const vk::ExtensionProperties &Extension)
		                           { return std::string_view{Extension.extensionName} == ExtensionName; });
	}

	inline bool InUse() const
	{
		return bIsEnabled;
	};

	const char *GetExtensionName() const
	{
		return ExtensionName;
	}

  protected:
	const char *ExtensionName;
	bool bIsEnabled = false;
};

class FVulkanInstanceExtension : public FVulkanExtensionBase
{
  public:
	FVulkanInstanceExtension(const char *InExtensionName, bool bIsInEnable)
	    : FVulkanExtensionBase(InExtensionName, bIsInEnable) {};

	static std::vector<vk::ExtensionProperties> GetDriverSupportedInstanceExtensions();
	static std::vector<vk::ExtensionProperties> GetLayerSupportedInstanceExtensions(const char *LayerName);
	static FVulkanInstanceExtensionArray GetWindSupportedInstanceExtensions(uint32 ApiVersion);
};

class FVulkanDeviceExtension : public FVulkanExtensionBase
{
  public:
	FVulkanDeviceExtension(FVulkanDevice *InDevice, const char *InExtensionName, bool bIsInEnable)
	    : FVulkanExtensionBase(InExtensionName, bIsInEnable), Device(InDevice) {};

	static std::vector<vk::ExtensionProperties> GetLayerSupportedInstanceExtensions(vk::PhysicalDevice Gpu,
	                                                                                const char *LayerName);

	static FVulkanDeviceExtensionArray GetWindSupportedDeviceExtensions(FVulkanDevice *InDevice, uint32 ApiVersion);
	static std::vector<vk::ExtensionProperties> GetDriverSupportedDeviceExtensions(vk::PhysicalDevice Gpu);

	virtual void PrePhysicalDeviceProperties(vk::PhysicalDeviceProperties2 &PhysicalDeviceProperties2)
	{
	}
	virtual void PostPhysicalDeviceProperties(FOptionalVulkanDeviceExtensions &ExtensionFlags)
	{
	}

	virtual void PrePhysicalDeviceFeatures(vk::PhysicalDeviceFeatures2 &PhysicalDeviceFeatures2)
	{
	}

	virtual void PostPhysicalDeviceFeatures(FOptionalVulkanDeviceExtensions &ExtensionFlags)
	{
	}

	virtual void PreCreateDevice(vk::DeviceCreateInfo &DeviceCreateInfo) {};

  protected:
	struct FOptionalVulkanDeviceExtensionProperties &GetDeviceExtensionProperties();
	bool bRequirementsPassed;

  private:
	FVulkanDevice *Device;
};

static WIND_INLINE std::vector<const char *> GetWindInstanceExtensions()
{
	std::vector<const char *> WindExtensions;
	WindExtensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef WIND_VULKAN_VALIDATION
	WindExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	FVulkanPlatform::GetInstanceExtensions(WindExtensions);

	return WindExtensions;
}

static WIND_INLINE std::vector<const char *> GetWindDeviceExtensions()
{
	std::vector<const char *> WindExtensions;

	WindExtensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	WindExtensions.emplace_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
	WindExtensions.emplace_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);

	FVulkanPlatform::GetDeviceExtensions(WindExtensions);

	return WindExtensions;
}
