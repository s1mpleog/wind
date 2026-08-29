#include "VulkanExtension.hpp"

#include "Core/Public/Check.hpp"
#include "vulkan/vulkan.hpp"

#include <string>
#include <vulkan/vulkan_core.h>

std::vector<vk::ExtensionProperties> FVulkanInstanceExtension::GetDriverSupportedInstanceExtensions()
{
	VERIFYVULKANRESULT_UNWRAP(OutInstanceExtensions, vk::enumerateInstanceExtensionProperties());

	CHECK(!OutInstanceExtensions.empty(),
	      "Vulkan loader returns 0 instance extensions enging can not be initialized with 0 instance extensions");

	return OutInstanceExtensions;
}

std::vector<vk::ExtensionProperties>
FVulkanInstanceExtension::GetLayerSupportedInstanceExtensions(const std::string_view LayerName)
{
	VERIFYVULKANRESULT_UNWRAP(OutInstanceExtensions, vk::enumerateInstanceExtensionProperties(std::string{LayerName}));
	return OutInstanceExtensions;
}

std::vector<vk::ExtensionProperties> FVulkanDeviceExtension::GetDriverSupportedDeviceExtensions(vk::PhysicalDevice Gpu)
{
	VERIFYVULKANRESULT_UNWRAP(OutDeviceExtensions, Gpu.enumerateDeviceExtensionProperties());

	CHECK(!OutDeviceExtensions.empty(),
	      "Device Does not supports any Extension Engine needs some extensions to continue...");

	return OutDeviceExtensions;
}

std::vector<vk::ExtensionProperties>
FVulkanDeviceExtension::GetLayerSupportedInstanceExtensions(vk::PhysicalDevice Gpu, const std::string_view LayerName)
{
	VERIFYVULKANRESULT_UNWRAP(OutDeviceExtensions, Gpu.enumerateDeviceExtensionProperties(std::string{LayerName}));
	return OutDeviceExtensions;
}
