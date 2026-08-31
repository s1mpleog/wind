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

class FVulkanDevice
{
  public:
	FVulkanDevice(vk::PhysicalDevice Gpu);

  private:
	vk::raii::Device Device{nullptr};
	vk::PhysicalDevice Gpu{};
};
