#pragma once

#include "Check.hpp"
#include "Definitions.hpp"
#include "VulkanGenericPlatform.h"
#include "VulkanQueue.hpp"
#include "vulkan/vulkan.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

class FVulkanAllocator;

struct FOptionalVulkanDeviceExtensions
{
	union
	{
		struct
		{
			uint64_t HasMemoryBudget : 1;
			uint64_t HasKHRMaintenance4 : 1;
			uint64_t HasKHRPushDescriptors : 1;
			uint64_t HasEXTLoadStoreOpNone : 1;
			uint64_t HasEXTHostImageCopy : 1;
			uint64_t HasExtDescriptorIndexing : 1;
		};
		uint64_t Packed;
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

	void Query(vk::PhysicalDevice PhysicalDevice, uint32_t APIVersion);

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

	void Destroy();

	void InitGpu(const vk::Instance InInstance) noexcept;

	vk::Device GetHandle() const
	{
		return Device;
	}

	bool HasTransferQueue() const
	{
		return Queues[(uint32_t)EVulkanQueueType::Transfer] != nullptr;
	}

	const std::vector<vk::QueueFamilyProperties2> GetQueueFamilyProps() const
	{
		return QueueFamilyProps;
	}

	vk::PhysicalDevice GetPhysicalHandle() const
	{
		return Gpu;
	}

	FVulkanQueue *GetGraphicsQueue() const
	{
		return Queues[(uint32_t)EVulkanQueueType::Graphics].get();
	}

	FVulkanQueue *GetPresentQueue() const
	{
		return PresentQueue;
	}

	void WaitUntilIdle()
	{
		// todo: handle error ?
		Device.waitIdle();
	}

	FVulkanQueue *GetQueue(EVulkanQueueType QueueType)
	{
		if (QueueType == EVulkanQueueType::Graphics)
		{
			return Queues[(uint32_t)EVulkanQueueType::Graphics].get();
		}
		else if (QueueType == EVulkanQueueType::Transfer)
		{
			return Queues[(uint32_t)EVulkanQueueType::Transfer].get();
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
	void CreateDevice();

	FVulkanPhysicalDeviceFeatures PhysicalDeviceFeatures;
	EGpuVendorId VendorId = EGpuVendorId::NotQueried;

	FOptionalVulkanDeviceExtensions OptionalDeviceExtensions;
	FOptionalVulkanDeviceExtensionProperties OptionalDeviceExtensionsProperties;

	vk::PhysicalDeviceIDProperties GpuIdProps;
	VkPhysicalDeviceProperties GpuProps;

	std::vector<vk::QueueFamilyProperties2> QueueFamilyProps;

	vk::Device Device{VK_NULL_HANDLE};
	vk::PhysicalDevice Gpu{VK_NULL_HANDLE};

	std::array<std::unique_ptr<FVulkanQueue>, (uint32_t)EVulkanQueueType::Count> Queues;
	FVulkanQueue *PresentQueue = nullptr;

	std::vector<const char *> DeviceExtensions;

	std::unique_ptr<FVulkanAllocator> Allocator;
};
