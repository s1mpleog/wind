#include "VulkanDevice.h"

#include "Check.hpp"
#include "Definitions.hpp"
#include "VulkanCheck.hpp"
#include "VulkanExtension.hpp"
#include "VulkanQueue.hpp"
#include "spdlog/spdlog.h"
#include "vulkan/vulkan.hpp"

#include <memory>
#include <optional>
#include <ranges>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_to_string.hpp>

void FVulkanPhysicalDeviceFeatures::Query(vk::PhysicalDevice PhysicalDevice, uint32_t APIVersion)
{
	if (APIVersion >= vk::ApiVersion13)
	{
		Core_1_2.pNext = &Core_1_3;
	}

	if (APIVersion >= vk::ApiVersion14)
	{
		Core_1_3.pNext = &Core_1_4;
	}
}

static bool IsExtensionAvailable(vk::PhysicalDevice Gpu, std::string_view RequestedExtension)
{
	// todo: can we ignore error ?
	std::vector<vk::ExtensionProperties> DeviceExtensions = Gpu.enumerateDeviceExtensionProperties().value();

	return std::ranges::any_of(DeviceExtensions, [&](const vk::ExtensionProperties &Extension)
	                           { return std::string_view{Extension.extensionName} == RequestedExtension; });
};

// later FenceManager, MemoryManager
FVulkanDevice::FVulkanDevice(vk::PhysicalDevice InGpu) : Device(VK_NULL_HANDLE), Gpu(InGpu)
{
	vk::PhysicalDeviceProperties2 PhysicalDeviceProperties2{};
	PhysicalDeviceProperties2.pNext = &GpuIdProps;

	Gpu.getProperties2(&PhysicalDeviceProperties2);

	GpuProps = PhysicalDeviceProperties2.properties;

	VendorId = ConvertToGpuVendorId(GpuProps.vendorID);

	WIND_LOG(info, "- DeviceName: {}", std::string_view{GpuProps.deviceName});
	WIND_LOG(info, "- API={}.{}.{} Driver = {}, VendorId = {}", vk::apiVersionMajor(GpuProps.apiVersion),
	         vk::apiVersionMinor(GpuProps.apiVersion), vk::apiVersionPatch(GpuProps.apiVersion), GpuProps.driverVersion,
	         GpuProps.vendorID);
	WIND_LOG(info, "- Max Descriptor Sets Bound = {}", GpuProps.limits.maxBoundDescriptorSets);
}

void FVulkanDevice::CreateDevice()
{
	CHECK(Device == VK_NULL_HANDLE);

	// optional
	vk::PhysicalDevicePageableDeviceLocalMemoryFeaturesEXT PageableMemory{};
	vk::PhysicalDeviceMemoryPriorityFeaturesEXT MemoryPriority{};

	// enable synchronization 2 and dynamic rendering since we are using profile we don't need to check for support
	vk::PhysicalDeviceVulkan13Features Features13{};
	Features13.dynamicRendering = vk::True;
	Features13.synchronization2 = vk::True;

	vk::PhysicalDeviceSwapchainMaintenance1FeaturesEXT Maintenance1{};
	Maintenance1.swapchainMaintenance1 = vk::True;

	if (IsExtensionAvailable(Gpu, VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME) &&
	    IsExtensionAvailable(Gpu, VK_EXT_PAGEABLE_DEVICE_LOCAL_MEMORY_EXTENSION_NAME))
	{
		MemoryPriority.memoryPriority = vk::True;
		PageableMemory.pageableDeviceLocalMemory = vk::True;

		WIND_LOG(info,
		         "Enabling optional extensions: MEMORY_PRIORITY_EXTENSION and PAGEABLE_DEVICE_LOCAL_MEMORY_EXTENSION");

		Maintenance1.pNext = &MemoryPriority;
		MemoryPriority.pNext = &PageableMemory;
	}

	Features13.pNext = &Maintenance1;

	vk::DeviceCreateInfo DeviceInfo{};

	DeviceInfo.pNext = &Features13;

	std::vector<vk::DeviceQueueCreateInfo> QueueFamilyInfos;

	std::optional<uint32_t> GraphicsFamilyIndex;
	std::optional<uint32_t> TransferFamilyIndex;

	const float QueuePriorites = 1.0F;

	WIND_LOG(info, "Found {} Queue Families", QueueFamilyProps.size());

	for (uint32_t FamilyIndex = 0; FamilyIndex < QueueFamilyProps.size(); ++FamilyIndex)
	{
		bool bIsValidQueue = false;

		const vk::QueueFamilyProperties2 &CurrProps = QueueFamilyProps[FamilyIndex];

		if (CurrProps.queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			if (!GraphicsFamilyIndex)
			{
				GraphicsFamilyIndex = FamilyIndex;
				bIsValidQueue = true;
			}
		}

		if (CurrProps.queueFamilyProperties.queueFlags & vk::QueueFlagBits::eTransfer)
		{
			if (!TransferFamilyIndex && !(CurrProps.queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics) &&
			    !(CurrProps.queueFamilyProperties.queueFlags & vk::QueueFlagBits::eCompute))
			{
				TransferFamilyIndex = FamilyIndex;
				bIsValidQueue = true;
			}
		}

		if (!bIsValidQueue)
		{
			continue;
		}

		// TODO: for now just use one queue from queue families later if need multiple queues then update
		// queueCount to CurrProps.queueCount
		QueueFamilyInfos.emplace_back(vk::DeviceQueueCreateFlags{}, FamilyIndex, 1, &QueuePriorites, nullptr);
	}

	CHECK(!GraphicsFamilyIndex.value(), "Failed to find graphics queue for engine we need graphics queue for rendering "
	                                    "can't continue without it... ");

	DeviceInfo.queueCreateInfoCount = QueueFamilyInfos.size();
	DeviceInfo.pQueueCreateInfos = QueueFamilyInfos.data();

	std::vector<const char *> WindDeviceExtensions = GetWindDeviceExtensions();

	// enable pageable device-local memory when available
	if (IsExtensionAvailable(Gpu, VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME) &&
	    IsExtensionAvailable(Gpu, VK_EXT_PAGEABLE_DEVICE_LOCAL_MEMORY_EXTENSION_NAME))
	{
		WindDeviceExtensions.push_back(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME);
		WindDeviceExtensions.push_back(VK_EXT_PAGEABLE_DEVICE_LOCAL_MEMORY_EXTENSION_NAME);
	}

	DeviceInfo.enabledExtensionCount = WindDeviceExtensions.size();
	DeviceInfo.ppEnabledExtensionNames = WindDeviceExtensions.data();

	vk::ResultValueType<vk::Device>::type DeviceResult = Gpu.createDevice(DeviceInfo);

	if (!DeviceResult && DeviceResult.error() == vk::Result::eErrorInitializationFailed)
	{
		FATAL("Cannot create a Vulkan device. Try updating your driver to latest version Error code: {}",
		      vk::to_string(DeviceResult.error()));
	}

	CHECK(DeviceResult.has_value());

	Device = DeviceResult.value();

	volkLoadDevice(Device);

	WIND_LOG(info, "Logical Device Created Successfully");

	// we are sure that graphics queue will exist
	Queues[(uint32_t)EVulkanQueueType::Graphics] =
	    std::make_unique<FVulkanQueue>(*this, GraphicsFamilyIndex.value(), EVulkanQueueType::Graphics);

	// transfer queue can be null if not found
	if (TransferFamilyIndex)
	{
		Queues[(uint32_t)EVulkanQueueType::Transfer] =
		    std::make_unique<FVulkanQueue>(*this, TransferFamilyIndex.value(), EVulkanQueueType::Transfer);
	}
}

void FVulkanDevice::InitGpu(const vk::Instance InInstance) noexcept
{
	QueueFamilyProps = Gpu.getQueueFamilyProperties2();
	CHECK(QueueFamilyProps.size() >= 1, "Vulkan return zero queues this should not happen on normal GPU");

	// TODO: later take version from somewhere else
	PhysicalDeviceFeatures.Query(Gpu, vk::ApiVersion13);

	CreateDevice();

	// create allocator
	Allocator = FVulkanAllocator(InInstance, Gpu, Device);
}

void FVulkanDevice::Destroy()
{
	if (Device != VK_NULL_HANDLE)
	{
		Device.destroy();
	}
}
