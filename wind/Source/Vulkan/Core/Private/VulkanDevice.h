#pragma once

#include "Check.hpp"
#include "Vulkan/Core/Private/VulkanGenericPlatform.h"
#include "Vulkan/Core/Private/VulkanQueue.hpp"
#include "Vulkan/Core/Public/Definitions.hpp"
#include "vulkan/vulkan.hpp"

#include <inplace_vector>
#include <memory>
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

	FVulkanDevice(const FVulkanDevice &) = delete;
	FVulkanDevice &operator=(const FVulkanDevice &) = delete;

	FVulkanDevice(FVulkanDevice &&) = default;
	FVulkanDevice &operator=(FVulkanDevice &&) = default;

	~FVulkanDevice();

	void InitGpu() WIND_NOEXCEPT;

	vk::Device GetHandle() const
	{
		return Device;
	}

	bool HasTransferQueue() const
	{
		return Queues[(uint32)EVulkanQueueType::Transfer] != nullptr;
	}

	const std::vector<vk::QueueFamilyProperties> GetQueueFamilyProps() const
	{
		return QueueFamilyProps;
	}

	vk::PhysicalDevice GetPhysicalHandle() const
	{
		return Gpu;
	}

	FVulkanQueue *GetGraphicsQueue() const
	{
		return Queues[(uint32)EVulkanQueueType::Graphics].get();
	}

	FVulkanQueue *GetPresentQueue() const
	{
		return PresentQueue;
	}

	void WaitUntilIdle()
	{
	}

	FVulkanQueue *GetQueue(EVulkanQueueType QueueType)
	{
		if (QueueType == EVulkanQueueType::Graphics)
		{
			return Queues[(uint32)EVulkanQueueType::Graphics].get();
		}
		else if (QueueType == EVulkanQueueType::Transfer)
		{
			return Queues[(uint32)EVulkanQueueType::Transfer].get();
		}

		return nullptr;
	}

	EGpuVendorId GetVendorId() const
	{
		return VendorId;
	}

	const FOptionalVulkanDeviceExtensionProperties &GetOptionalExtensionProperties() const
	{
		return OptionalDeviceExtensionsProperties;
	}

	void SetupPresentQueue(vk::SurfaceKHR Surface);

  private:
	// void CreateDevice(FVulkanDeviceExtensionArray &WindExtensions);
	 void CreateDevice();

	FVulkanPhysicalDeviceFeatures PhysicalDeviceFeatures;
	EGpuVendorId VendorId = EGpuVendorId::NotQueried;

	FOptionalVulkanDeviceExtensions OptionalDeviceExtensions;
	FOptionalVulkanDeviceExtensionProperties OptionalDeviceExtensionsProperties;

	vk::PhysicalDeviceIDProperties GpuIdProps;
	VkPhysicalDeviceProperties GpuProps;

	std::vector<vk::QueueFamilyProperties> QueueFamilyProps;

	vk::Device Device{VK_NULL_HANDLE};
	vk::PhysicalDevice Gpu{VK_NULL_HANDLE};

	std::inplace_vector<std::unique_ptr<FVulkanQueue>, (uint32)EVulkanQueueType::Count> Queues;
	FVulkanQueue *PresentQueue = nullptr;

	std::vector<const char *> DeviceExtensions;
};
