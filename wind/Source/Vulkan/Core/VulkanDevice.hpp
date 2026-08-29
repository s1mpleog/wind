#pragma once

#include "Config.hpp"
#include "Types.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Configuration.hpp"
#include "vulkan/vulkan.hpp"

#include <optional>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

struct FGpuDevice
{
	vk::raii::PhysicalDevice PhysicalDevice{nullptr};
	vk::raii::Device Device{nullptr};
	vk::raii::Queue GraphicsQueue{nullptr};
	vk::raii::Queue PresentationQueue{nullptr};
	std::optional<vk::raii::Queue> TransferQueue;
	std::optional<uint32> GraphicsQueueIdx;
	std::optional<uint32> PresentationQueueIdx;
	std::optional<uint32> TransferQueueIdx;
	vk::PhysicalDeviceProperties PhysicalDeviceProps{};
	vk::raii::CommandPool GraphicsPool{nullptr};
	std::optional<vk::raii::CommandPool> TransferPool;

	WIND_NODISCARD constexpr auto HasTransferQueue() const WIND_NOEXCEPT -> bool

	{
		return TransferQueueIdx.has_value() && TransferQueue.has_value();
	}
};

WIND_NODISCARD auto DeviceCreate(const FConfiguration &Cfg, const vk::raii::Instance &Instance,
                                 const vk::raii::SurfaceKHR &Surface) WIND_NOEXCEPT -> TWindResult<FGpuDevice>;

class FVulkanPhysicalDeviceFeatures
{
  public:
	auto Query(vk::PhysicalDevice PhysicalDevice, uint32 APIVersion) WIND_NOEXCEPT -> void;

  private:
	vk::PhysicalDeviceVulkan12Features Core_1_2;
	vk::PhysicalDeviceVulkan13Features Core_1_3;
	vk::PhysicalDeviceVulkan14Features Core_1_4;
};

class FVulkanDevice
{
  public:
	WIND_NODISCARD auto CreateDevice() WIND_NOEXCEPT -> TWindResult<void>;

	WIND_NODISCARD auto GetDevice() const -> const vk::raii::Device &
	{
		return Device;
	}

  private:
	vk::raii::Device Device;
	FVulkanPhysicalDeviceFeatures PhysicalDeviceFeatures;
};
