#pragma once

#include "Vulkan/Core/Private/VulkanGenericPlatform.h"
#include "Vulkan/Core/Public/Definitions.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_raii.hpp"

#include <vector>
#include <vulkan/vulkan_core.h>

struct FOptionalVulkanDeviceExtensions
{
	union
	{
		struct
		{
			uint64 HasMemoryBudget : 1;
			uint64 HasKHRMaintenance4 : 1;
			uint64 HasKHRPushDescriptors : 1;
			uint64 HasEXTLoadStoreOpNone : 1;
			uint64 HasEXTHostImageCopy : 1;
			uint64 HasExtDescriptorIndexing : 1;
			uint64 HasExtDynamicRendering : 1;
			uint64 HasKHRSynchronization2 : 1;
		};
		uint64 Packed;
	};

	FOptionalVulkanDeviceExtensions() : Packed(0) {};
};

struct FOptionalVulkanDeviceExtensionProperties
{
	FOptionalVulkanDeviceExtensionProperties()
	{
		std::memset(this, 0, sizeof(*this));
	}

	vk::PhysicalDeviceDriverPropertiesKHR PhysicalDeviceDriverProperties;
	vk::PhysicalDeviceMaintenance4PropertiesKHR PhysicalDeviceMaintenance4Properties;
	vk::PhysicalDevicePushDescriptorPropertiesKHR PhysicalDevicePushDescriptorProperties;
};

class FVulkanPhysicalDeviceFeatures
{
  public:
	FVulkanPhysicalDeviceFeatures()
	{
		std::memset(this, 0, sizeof(*this));
	}

	void Query(vk::PhysicalDevice PhysicalDevice, uint32 APIVersion);

  private:
	vk::PhysicalDeviceVulkan12Features Core_1_2;
	vk::PhysicalDeviceVulkan13Features Core_1_3;
	vk::PhysicalDeviceVulkan14Features Core_1_4;
};

class FVulkanDevice
{
  public:
	FVulkanDevice(vk::PhysicalDevice Gpu);
	~FVulkanDevice();

	void InitGpu(const vk::raii::Context &InContext) WIND_NOEXCEPT;

	const FOptionalVulkanDeviceExtensionProperties &GetOptionalExtensionProperties() const
	{
		return OptionalDeviceExtensionsProperties;
	}

  private:
	void CreateDevice(FVulkanDeviceExtensionArray &WindExtensions, const vk::raii::Context &InContext);

	FVulkanPhysicalDeviceFeatures PhysicalDeviceFeatures;
	EGpuVendorId VendorId = EGpuVendorId::NotQueried;

	FOptionalVulkanDeviceExtensions OptionalDeviceExtensions;
	FOptionalVulkanDeviceExtensionProperties OptionalDeviceExtensionsProperties;

	vk::PhysicalDeviceIDProperties GpuIdProps;
	VkPhysicalDeviceProperties GpuProps;

	std::vector<vk::QueueFamilyProperties> QueueFamilyProps;

	vk::Device Device{VK_NULL_HANDLE};
	vk::PhysicalDevice Gpu{VK_NULL_HANDLE};

	std::vector<const char *> DeviceExtensions;
};
