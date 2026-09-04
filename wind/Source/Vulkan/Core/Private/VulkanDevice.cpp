#include "VulkanDevice.h"

#include "Check.hpp"
#include "Vulkan/Core/Private/VulkanExtension.hpp"
#include "Vulkan/Core/Private/VulkanQueue.hpp"
#include "Vulkan/Core/Public/Definitions.hpp"
#include "spdlog/spdlog.h"
#include "vulkan/vulkan.hpp"

#include <memory>
#include <optional>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_to_string.hpp>

void FVulkanPhysicalDeviceFeatures::Query(vk::PhysicalDevice PhysicalDevice, uint32 APIVersion)
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

// later FenceManager, MemoryManager
FVulkanDevice::FVulkanDevice(vk::PhysicalDevice InGpu) : Device(VK_NULL_HANDLE), Gpu(InGpu)
{
	vk::PhysicalDeviceProperties2 PhysicalDeviceProperties2{};
	PhysicalDeviceProperties2.pNext = &GpuIdProps;

	Gpu.getProperties2(&PhysicalDeviceProperties2);

	GpuProps = PhysicalDeviceProperties2.properties;

	VendorId = ConvertToGpuVendorId(GpuProps.vendorID);

	spdlog::info("- DeviceName: {}", std::string_view{GpuProps.deviceName});
	spdlog::info("- API={}.{}.{} Driver = {}, VendorId = {}", vk::apiVersionMajor(GpuProps.apiVersion),
	             vk::apiVersionMinor(GpuProps.apiVersion), vk::apiVersionPatch(GpuProps.apiVersion),
	             GpuProps.driverVersion, GpuProps.vendorID);
	spdlog::info("- Max Descriptor Sets Bound = {}", GpuProps.limits.maxBoundDescriptorSets);
}

void FVulkanDevice::CreateDevice()
{
	CHECK(Device == VK_NULL_HANDLE);

	vk::DeviceCreateInfo DeviceInfo{};

	std::vector<vk::DeviceQueueCreateInfo> QueueFamilyInfos;

	std::optional<uint32> GraphicsFamilyIndex;
	std::optional<uint32> TransferFamilyIndex;

	const float QueuePriorites = 1.0F;

	WIND_LOG(info, "Found {} Queue Families", QueueFamilyProps.size());

	for (uint32 FamilyIndex = 0; FamilyIndex < QueueFamilyProps.size(); ++FamilyIndex)
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

	Queues.resize((uint32)EVulkanQueueType::Count);

	// we are sure that graphics queue will exist
	Queues[(uint32)EVulkanQueueType::Graphics] =
	    std::make_unique<FVulkanQueue>(*this, GraphicsFamilyIndex.value(), EVulkanQueueType::Graphics);

	// transfer queue can be null if not found
	if (TransferFamilyIndex)
	{
		Queues[(uint32)EVulkanQueueType::Transfer] =
		    std::make_unique<FVulkanQueue>(*this, TransferFamilyIndex.value(), EVulkanQueueType::Transfer);
	}
}

void FVulkanDevice::InitGpu() WIND_NOEXCEPT
{
	QueueFamilyProps = Gpu.getQueueFamilyProperties2();
	CHECK(QueueFamilyProps.size() >= 1, "Vulkan return zero queues this should not happen on normal GPU");

	// TODO: later take version from somewhere else
	PhysicalDeviceFeatures.Query(Gpu, vk::ApiVersion13);

	CreateDevice();
}

void FVulkanDevice::Destroy()
{
	if (Device != VK_NULL_HANDLE)
	{
		Device.destroy();
	}
}
