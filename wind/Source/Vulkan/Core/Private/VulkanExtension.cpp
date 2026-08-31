#include "VulkanExtension.hpp"

#include "Core/Public/Check.hpp"
#include "Vulkan/Core/Private/VulkanDevice.h"
#include "Vulkan/Core/Private/VulkanGenericPlatform.h"
#include "vulkan/vulkan.hpp"

#include <memory>
#include <string>
#include <vulkan/vulkan_core.h>

struct FOptionalVulkanDeviceExtensionProperties &FVulkanDeviceExtension::GetDeviceExtensionProperties()
{
	const FOptionalVulkanDeviceExtensionProperties &ExtensionProperties = Device->GetOptionalExtensionProperties();
	return const_cast<FOptionalVulkanDeviceExtensionProperties &>(ExtensionProperties);
}

std::vector<vk::ExtensionProperties> FVulkanInstanceExtension::GetDriverSupportedInstanceExtensions()
{
	VERIFYVULKANRESULT_UNWRAP(OutInstanceExtensions, vk::enumerateInstanceExtensionProperties());

	CHECK(!OutInstanceExtensions.empty(),
	      "Vulkan loader returns 0 instance extensions engine can not be initialized with 0 instance extensions");

	return OutInstanceExtensions;
}

std::vector<vk::ExtensionProperties>
FVulkanInstanceExtension::GetLayerSupportedInstanceExtensions(const char *LayerName)
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
std::vector<vk::ExtensionProperties> FVulkanDeviceExtension::GetLayerSupportedInstanceExtensions(vk::PhysicalDevice Gpu,
                                                                                                 const char *LayerName)
{
	VERIFYVULKANRESULT_UNWRAP(OutDeviceExtensions, Gpu.enumerateDeviceExtensionProperties(std::string{LayerName}));
	return OutDeviceExtensions;
}

class FVulkanEXTDescriptorIndexingExtension : public FVulkanDeviceExtension
{
  public:
	FVulkanEXTDescriptorIndexingExtension(FVulkanDevice *InDevice)
	    : FVulkanDeviceExtension(InDevice, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, true) {};

	virtual void PrePhysicalDeviceFeatures(vk::PhysicalDeviceFeatures2 &PhysicalDeviceFeatures2) override final
	{
		DescriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		DescriptorIndexingFeatures.pNext = PhysicalDeviceFeatures2.pNext;
		PhysicalDeviceFeatures2.pNext = &DescriptorIndexingFeatures;
	}

	virtual void PostPhysicalDeviceFeatures(FOptionalVulkanDeviceExtensions &ExtensionFlags) override final
	{
		bRequirementsPassed = (DescriptorIndexingFeatures.runtimeDescriptorArray == VK_TRUE) &&
		                      (DescriptorIndexingFeatures.descriptorBindingPartiallyBound == VK_TRUE) &&
		                      (DescriptorIndexingFeatures.descriptorBindingUpdateUnusedWhilePending == VK_TRUE) &&
		                      (DescriptorIndexingFeatures.descriptorBindingVariableDescriptorCount == VK_TRUE);

		ExtensionFlags.HasExtDescriptorIndexing = bRequirementsPassed;
	}

	virtual void PreCreateDevice(vk::DeviceCreateInfo &DeviceCreateInfo) override final
	{
		if (bRequirementsPassed)
		{
			DeviceCreateInfo.pNext = &DescriptorIndexingFeatures;
		}
	}

	VkPhysicalDeviceDescriptorIndexingFeatures DescriptorIndexingFeatures;
};

class FVulkanKHRGetMemoryRequirements2Extension : public FVulkanDeviceExtension
{
  public:
	FVulkanKHRGetMemoryRequirements2Extension(FVulkanDevice *InDevice)
	    : FVulkanDeviceExtension(InDevice, VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME, true) {};
};

class FVulkanKHRDyamicRenderingExtension : public FVulkanDeviceExtension
{
  public:
	FVulkanKHRDyamicRenderingExtension(FVulkanDevice *InDevice)
	    : FVulkanDeviceExtension(InDevice, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, true) {};

	virtual void PrePhysicalDeviceFeatures(vk::PhysicalDeviceFeatures2 &PhysicalDeviceFeatures2) override final
	{
		DynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
		DynamicRenderingFeatures.pNext = PhysicalDeviceFeatures2.pNext;
		PhysicalDeviceFeatures2.pNext = &DynamicRenderingFeatures;
	}

	virtual void PostPhysicalDeviceFeatures(FOptionalVulkanDeviceExtensions &ExtensionFlags) override final
	{
		bRequirementsPassed = (DynamicRenderingFeatures.dynamicRendering == VK_TRUE);
		ExtensionFlags.HasExtDynamicRendering = bRequirementsPassed;
	}

	virtual void PreCreateDevice(vk::DeviceCreateInfo &DeviceCreateInfo) override final
	{
		if (bRequirementsPassed)
		{
			DeviceCreateInfo.pNext = &DynamicRenderingFeatures;
		}
	}

	VkPhysicalDeviceDynamicRenderingFeatures DynamicRenderingFeatures;
};

class FVulkanKHRSynchronization2 : public FVulkanDeviceExtension
{
  public:
	FVulkanKHRSynchronization2(FVulkanDevice *InDevice)
	    : FVulkanDeviceExtension(InDevice, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME, true) {};

	virtual void PrePhysicalDeviceFeatures(vk::PhysicalDeviceFeatures2 &PhysicalDeviceFeatures2) override final
	{
		Synchronization2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
		Synchronization2Features.pNext = PhysicalDeviceFeatures2.pNext;
		PhysicalDeviceFeatures2.pNext = &Synchronization2Features;
	}

	virtual void PostPhysicalDeviceFeatures(FOptionalVulkanDeviceExtensions &ExtensionFlags) override final
	{
		bRequirementsPassed = (Synchronization2Features.synchronization2 == VK_TRUE);
		ExtensionFlags.HasKHRSynchronization2 = bRequirementsPassed;
	}

	virtual void PreCreateDevice(vk::DeviceCreateInfo &DeviceCreateInfo) override final
	{
		if (bRequirementsPassed)
		{
			DeviceCreateInfo.pNext = &Synchronization2Features;
		}
	}

	VkPhysicalDeviceSynchronization2Features Synchronization2Features;
};

FVulkanDeviceExtensionArray FVulkanDeviceExtension::GetWindSupportedDeviceExtensions(FVulkanDevice *InDevice,
                                                                                     uint32 ApiVersion)
{
	FVulkanDeviceExtensionArray OutWindExtensions;

#define ADD_SIMPLE_EXTENSION(EXTENSION_NAME, IS_ENABLED)                                                               \
	OutWindExtensions.push_back(std::make_unique<FVulkanDeviceExtension>(InDevice, EXTENSION_NAME, IS_ENABLED));

#define ADD_CUSTOM_EXTENSION(EXTENSION_CLASS) OutWindExtensions.push_back(std::make_unique<EXTENSION_CLASS>(InDevice));

	ADD_SIMPLE_EXTENSION(VK_KHR_SWAPCHAIN_EXTENSION_NAME, true);
	ADD_SIMPLE_EXTENSION(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME, true);
	ADD_SIMPLE_EXTENSION(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME, true);

	// Dynamic Rendering and Synchronization2 has been promoted in version 1.4
	// so only set if ApiVersion is less than 1.4
	if (ApiVersion < VK_API_VERSION_1_4)
	{
		ADD_CUSTOM_EXTENSION(FVulkanKHRDyamicRenderingExtension);
		ADD_CUSTOM_EXTENSION(FVulkanKHRSynchronization2);
	}

	ADD_CUSTOM_EXTENSION(FVulkanEXTDescriptorIndexingExtension);
	ADD_CUSTOM_EXTENSION(FVulkanKHRGetMemoryRequirements2Extension);

	return OutWindExtensions;
};

class FVulkanKHRSurfaceMaintenance1Extension : public FVulkanInstanceExtension
{
  public:
	FVulkanKHRSurfaceMaintenance1Extension()
	    : FVulkanInstanceExtension(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME, true) {};
};

class FVulkanKHRSurfaceCapabilities2ExtensionName : public FVulkanInstanceExtension
{
  public:
	FVulkanKHRSurfaceCapabilities2ExtensionName()
	    : FVulkanInstanceExtension(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME, true) {};
};

FVulkanInstanceExtensionArray FVulkanInstanceExtension::GetWindSupportedInstanceExtensions(uint32 ApiVersion)
{
	FVulkanInstanceExtensionArray OutInstanceExtensions;

	OutInstanceExtensions.push_back(std::make_unique<FVulkanInstanceExtension>(VK_KHR_SURFACE_EXTENSION_NAME, true));

	OutInstanceExtensions.push_back(std::make_unique<FVulkanInstanceExtension>(VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	                                                                           WIND_VULKAN_VALIDATION ? true : false));

	OutInstanceExtensions.push_back(std::make_unique<FVulkanKHRSurfaceCapabilities2ExtensionName>());

	OutInstanceExtensions.push_back(std::make_unique<FVulkanKHRSurfaceMaintenance1Extension>());

	return OutInstanceExtensions;
}
