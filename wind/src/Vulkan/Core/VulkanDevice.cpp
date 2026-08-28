#include "Vulkan/Core/VulkanDevice.hpp"

#include "Error.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Configuration.hpp"
#include "Vulkan/Types.hpp"
#include "spdlog/spdlog.h"
#include "vulkan/vulkan.hpp"

#include <algorithm>
#include <optional>
#include <ranges>
#include <set>
#include <string_view>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

struct FPhysicalDeviceCandidate
{
	TU32 Score{};
	vk::raii::PhysicalDevice Device{nullptr};
	vk::PhysicalDeviceProperties DeviceProps{};
	std::optional<TU32> GraphicsQueueIndex;
	std::optional<TU32> PresentQueueIndex;
	std::optional<TU32> TransferQueueIndex;
};

static auto MakePhysicalDeviceCandidate(const vk::raii::PhysicalDevice &PhysicalDevice,
                                        const vk::raii::SurfaceKHR &Surface) -> FPhysicalDeviceCandidate
{
	std::optional<FPhysicalDeviceCandidate> Best{};

	[[maybe_unused]] auto Features2 = PhysicalDevice.getFeatures2().features;
	auto Properties2 = PhysicalDevice.getProperties2().properties;
	auto QueueInfos = PhysicalDevice.getQueueFamilyProperties();

	auto FindQueue = [&](vk::QueueFlagBits Flag) -> std::optional<unsigned int>
	{
		auto It = std::ranges::find_if(QueueInfos, [&](const vk::QueueFamilyProperties &QueueInfo) -> bool
		                               { return static_cast<bool>(QueueInfo.queueFlags & Flag); });
		return It != QueueInfos.end() ? std::optional{static_cast<TU32>(std::distance(QueueInfos.begin(), It))}
		                              : std::nullopt;
	};

	auto FindPresentQueue = [&](vk::SurfaceKHR Surface) -> std::optional<TU32>
	{
		for (TU32 I = 0; I < QueueInfos.size(); I++)
		{
			if (PhysicalDevice.getSurfaceSupportKHR(I, Surface).has_value())
				return I;
		}
		return std::nullopt;
	};

	auto GraphicsQueue = FindQueue(vk::QueueFlagBits::eGraphics);

	if (!GraphicsQueue)
		return {};

	auto PresentQueue = FindPresentQueue(Surface);

	if (!PresentQueue)
		return {};

	auto TransferQueue = FindQueue(vk::QueueFlagBits::eTransfer);

	FPhysicalDeviceCandidate Candidate{
	    .Score = 0,
	    .Device = PhysicalDevice,
	    .DeviceProps = Properties2,
	    .GraphicsQueueIndex = GraphicsQueue,
	    .PresentQueueIndex = PresentQueue,
	    .TransferQueueIndex = TransferQueue ? std::optional<TU32>{TransferQueue} : std::nullopt,
	};

	if (TransferQueue)
		Candidate.Score += 100;

	// if(features_2.samplerAnisotropy == vk::True)
	//   candidate.score += 100;

	if (Properties2.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
		Candidate.Score += 500;

	if (Properties2.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
		Candidate.Score += 300;

	if (Properties2.deviceType == vk::PhysicalDeviceType::eCpu)
		Candidate.Score += 150;

	if (!Best || Candidate.Score > Best->Score)
	{
		Best = Candidate;
	}

	return Best.value();
}

WIND_NODISCARD auto CheckHardRequirements(const FConfiguration &Cfg,
                                          const vk::raii::PhysicalDevice &Device) WIND_NOEXCEPT -> bool
{
	auto Features2 = Device.getFeatures2().features;
	auto Properties2 = Device.getProperties2().properties;

	auto Chain =
	    Device.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features,
	                        vk::PhysicalDeviceSwapchainMaintenance1FeaturesKHR, vk::PhysicalDeviceVulkan12Features>();

	auto &Features13 = Chain.get<vk::PhysicalDeviceVulkan13Features>();

	auto &MaintainanceFeature = Chain.get<vk::PhysicalDeviceSwapchainMaintenance1FeaturesKHR>();

	auto AvailableExtensions = Device.enumerateDeviceExtensionProperties();

	auto Features12 = Chain.get<vk::PhysicalDeviceVulkan12Features>();

	// limitation can't use TWindResult :(
	if (!AvailableExtensions || AvailableExtensions->empty())
		return false;

	const std::array<std::string_view, 3> RequiredExtensions = {
	    VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, VK_KHR_MAINTENANCE1_EXTENSION_NAME};

	// for all required extensions:
	// does any of available extensions matches the requirement
	// if available extensions = {"A", "B", "C"}
	// and required extensions = {"A", "B"}
	// then
	// "A" == "A" -> any_of true
	// "B" == "B" -> any_of true
	const auto Supported = std::ranges::all_of(
	    RequiredExtensions,
	    [&](std::string_view Required) -> bool
	    {
		    return std::ranges::any_of(*AvailableExtensions, [&](const vk::ExtensionProperties &Extension) -> bool
		                               { return Required == Extension.extensionName; });
	    });

	return (Features2.geometryShader == vk::True) && Properties2.apiVersion >= ToVk(Cfg.ApiVersion) &&
	       (Features13.dynamicRendering == vk::True) && (Features13.synchronization2 == vk::True) &&
	       (MaintainanceFeature.swapchainMaintenance1 == vk::True) && (Features12.runtimeDescriptorArray == vk::True) &&
	       Supported;
}

// responsible for selecting the preferred physical device
WIND_NODISCARD static auto SelectPhysicalDevice(const FConfiguration &Cfg, const vk::raii::Instance &Instance,
                                                const vk::raii::SurfaceKHR &Surface) WIND_NOEXCEPT
    -> TWindResult<FPhysicalDeviceCandidate>
{
	auto PhysicalDevices = WIND_TRY(Instance.enumeratePhysicalDevices(), ErrorCode::InternalError);

	WIND_ENSURE_NOT_EMPTY(PhysicalDevices, WindError::vulkan(ErrorCode::NoSuitablePhysicalDevice));

	auto UsableDevices = PhysicalDevices | std::views::filter([&](const vk::raii::PhysicalDevice &Device) -> bool
	                                                          { return CheckHardRequirements(Cfg, Device); });

	WIND_ENSURE_NOT_EMPTY(UsableDevices, WindError::vulkan(ErrorCode::NoSuitablePhysicalDevice));

	auto Candidates = UsableDevices | std::views::transform([&](const auto &Dev) -> auto
	                                                        { return MakePhysicalDeviceCandidate(Dev, Surface); });

	WIND_ENSURE_NOT_EMPTY(Candidates, WindError::vulkan(ErrorCode::NoSuitablePhysicalDevice));

	auto Best = std::ranges::max_element(Candidates, [](const auto &Lhs, const auto &Rhs) -> auto
	                                     { return Lhs.Score < Rhs.Score; });

	// we already checked candidates empty so at this point we are sure that there will be atleast one candidate
	auto SelectedGpu = *Best;

#ifdef WIND_LOG_ENABLE
	spdlog::info("using GPU: {}, graphics_queue: {}, presentation_queue: {}, transfer_queue: {}",
	             std::string_view{SelectedGpu.DeviceProps.deviceName}, SelectedGpu.GraphicsQueueIndex.value(),
	             SelectedGpu.PresentQueueIndex.value(), SelectedGpu.TransferQueueIndex.value());
#endif

	return SelectedGpu;
};

WIND_NODISCARD static auto CreateLogicalDevice(FPhysicalDeviceCandidate Candidate) WIND_NOEXCEPT
    -> TWindResult<FGpuDevice>
{
	vk::PhysicalDeviceVulkan12Features Features12{};
	Features12.runtimeDescriptorArray = vk::True;

	vk::PhysicalDeviceVulkan13Features Features13{};
	Features13.dynamicRendering = vk::True;
	Features13.synchronization2 = vk::True;

	vk::PhysicalDeviceSwapchainMaintenance1FeaturesEXT MaintenanceFeature{};
	MaintenanceFeature.swapchainMaintenance1 = vk::True;
	MaintenanceFeature.pNext = &Features12;

	Features13.pNext = &MaintenanceFeature;

	float QueuePriorities = 1.0F;

	std::set UniqueQueues{Candidate.GraphicsQueueIndex.value(), Candidate.PresentQueueIndex.value()};

	if (Candidate.TransferQueueIndex)
		UniqueQueues.insert(Candidate.TransferQueueIndex.value());

	std::vector<vk::DeviceQueueCreateInfo> QueueCreateInfos;
	QueueCreateInfos.reserve(UniqueQueues.size());

	for (auto Idx : UniqueQueues)
	{
		QueueCreateInfos.emplace_back(vk::DeviceQueueCreateFlags{}, Idx, 1, &QueuePriorities);
	}

#ifdef WIND_LOG_ENABLE
	spdlog::info("using {} queues", QueueCreateInfos.size());
#endif

	// NOTE: i am using this same array in two places i should put this somewhere maybe some DeviceConfiguration or
	// something
	const std::array<const char *, 3> EnabledExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	                                                       VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
	                                                       VK_KHR_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME};

	vk::DeviceCreateInfo CreateInfo{};
	CreateInfo.pNext = &Features13;
	CreateInfo.enabledExtensionCount = EnabledExtensions.size();
	CreateInfo.ppEnabledExtensionNames = EnabledExtensions.data();
	CreateInfo.queueCreateInfoCount = QueueCreateInfos.size();
	CreateInfo.pQueueCreateInfos = QueueCreateInfos.data();

	WIND_ASSERT(Candidate.Device != nullptr && "Physical device is null");

	auto Device = WIND_TRY(Candidate.Device.createDevice(CreateInfo), ErrorCode::FailedToCreateDevice);

#ifdef WIND_LOG_ENABLE
	spdlog::info("Creating logical device, enabled extensions: {}", EnabledExtensions.size());
#endif

	FGpuDevice GpuDevice{};

	GpuDevice.Device = std::move(Device);
	GpuDevice.PhysicalDevice = std::move(Candidate.Device);
	GpuDevice.PhysicalDeviceProps = Candidate.DeviceProps;
	GpuDevice.GraphicsQueueIdx = Candidate.GraphicsQueueIndex;
	GpuDevice.PresentationQueueIdx = Candidate.PresentQueueIndex;
	GpuDevice.GraphicsQueue = GpuDevice.Device.getQueue(GpuDevice.GraphicsQueueIdx.value(), 0);
	GpuDevice.PresentationQueue = GpuDevice.Device.getQueue(GpuDevice.PresentationQueueIdx.value(), 0);

	if (Candidate.TransferQueueIndex) [[likely]]
	{
		GpuDevice.TransferQueueIdx = Candidate.TransferQueueIndex;
		GpuDevice.PresentationQueue = GpuDevice.Device.getQueue(GpuDevice.TransferQueueIdx.value(), 0);
	}

	return GpuDevice;
};

WIND_NODISCARD static auto CreateCommandPool(const vk::raii::Device &Device, TU32 QueueIndex) WIND_NOEXCEPT
    -> TWindResult<vk::raii::CommandPool>
{
	vk::CommandPoolCreateInfo CmdPoolCreateInfo{};
	CmdPoolCreateInfo.queueFamilyIndex = QueueIndex;
	CmdPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

	return WIND_TRY(Device.createCommandPool(CmdPoolCreateInfo));
}

WIND_NODISCARD auto DeviceCreate(const FConfiguration &Cfg, const vk::raii::Instance &Instance,
                                 const vk::raii::SurfaceKHR &Surface) WIND_NOEXCEPT -> TWindResult<FGpuDevice>
{
	auto Candidate = WIND_TRY(SelectPhysicalDevice(Cfg, Instance, Surface));
	auto GpuDevice = WIND_TRY(CreateLogicalDevice(std::move(Candidate)));

	GpuDevice.GraphicsPool = WIND_TRY(CreateCommandPool(GpuDevice.Device, GpuDevice.GraphicsQueueIdx.value()));

	// TODO: think about this
	if (GpuDevice
	        .HasTransferQueue() /* && gpu_device.transfer_queue_idx.value() != gpu_device.graphics_queue_idx.value() */)
	{
		GpuDevice.TransferPool = WIND_TRY(CreateCommandPool(GpuDevice.Device, GpuDevice.TransferQueueIdx.value()));
	}

	return GpuDevice;
}

auto FVulkanPhysicalDeviceFeatures::Query(vk::PhysicalDevice PhysicalDevice, TU32 APIVersion) WIND_NOEXCEPT -> void
{
	vk::PhysicalDeviceFeatures2 PhysicalDeviceFeatures2 = PhysicalDevice.getFeatures2();
	vk::PhysicalDeviceProperties2 PhysicalDeviceProperties2 = PhysicalDevice.getProperties2();

	if (APIVersion >= vk::ApiVersion13)
	{
	}
}

WIND_NODISCARD auto FVulkanDevice::CreateDevice() WIND_NOEXCEPT -> TWindResult<void>
{
	PhysicalDeviceFeatures.Query({}, VK_API_VERSION_1_4);
	return {};
}
