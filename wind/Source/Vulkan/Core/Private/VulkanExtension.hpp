#pragma once

#include "vulkan/vulkan.hpp"

#include <algorithm>
#include <string_view>
#include <vector>

class FVulkanDevice;

class FVulkanExtensionBase
{
  public:
	explicit FVulkanExtensionBase(const std::string_view InExtensionName) : ExtensionName(InExtensionName) {};

	virtual ~FVulkanExtensionBase() = default;

	static bool FindExtension(const std::span<vk::ExtensionProperties> WindExtensions,
	                          const std::string_view ExtensionName)
	{
		return std::ranges::any_of(WindExtensions, [&](const vk::ExtensionProperties &Extension)
		                           { return std::string_view{Extension.extensionName} == ExtensionName; });
	}

  protected:
	const std::string_view ExtensionName;
};

class FVulkanInstanceExtension : public FVulkanExtensionBase
{
  public:
	explicit FVulkanInstanceExtension(const std::string_view InExtensionName)
	    : FVulkanExtensionBase(InExtensionName) {};

	static std::vector<vk::ExtensionProperties> GetDriverSupportedInstanceExtensions();

	static std::vector<vk::ExtensionProperties> GetLayerSupportedInstanceExtensions(const std::string_view LayerName);
};

class FVulkanDeviceExtension : public FVulkanExtensionBase
{
  public:
	FVulkanDeviceExtension(FVulkanDevice *InDevice, const std::string_view InExtensionName)
	    : FVulkanExtensionBase(InExtensionName), Device(InDevice) {};

	static std::vector<vk::ExtensionProperties> GetDriverSupportedDeviceExtensions(vk::PhysicalDevice Gpu);

	static std::vector<vk::ExtensionProperties> GetLayerSupportedInstanceExtensions(vk::PhysicalDevice Gpu,
	                                                                                const std::string_view LayerName);

	virtual void PrePhysicalDeviceProperties(vk::PhysicalDeviceProperties2KHR &PhysicalDeviceProperties2)
	{
	}
	virtual void PostPhysicalDeviceProperties(/* TODO: custom type FOptionalVulkanDeviceExtensions& */)
	{
	}

	virtual void PrePhysicalDeviceFeatures(vk::PhysicalDeviceProperties2KHR &PhysicalDeviceFeatures2)
	{
	}
	virtual void PostPhysicalDeviceFeatures(/*FOptionalVulkanDeviceExtensions &ExtensionFlags*/)
	{
	}

  private:
	FVulkanDevice *Device;
};
