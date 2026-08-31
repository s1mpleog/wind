#pragma once

#include "Vulkan/Core/Configuration.hpp"
#include "vulkan/vulkan_raii.hpp"

#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

class FVulkanDevice;

class FVulkanCore
{
  public:
	void Initialize() WIND_NOEXCEPT;

	FVulkanCore(FConfiguration &InConfig);

	// TODO: maybe prefer span
	const std::vector<const char *> &GetInstanceExtensions() const WIND_NOEXCEPT
	{
		return InstanceExtensions;
	};

	const std::vector<const char *> &GetInstanceLayers() const WIND_NOEXCEPT
	{
		return InstanceLayers;
	};

	const vk::raii::Instance &GetInstance() const WIND_NOEXCEPT
	{
		return Instance;
	}

	FVulkanDevice *GetDevice() const WIND_NOEXCEPT
	{
		return Device.get();
	}

	uint32 GetApiVersion() const
	{
		return ApiVersion;
	}

	// later set image layout, get viewports

  private:
	vk::raii::Context Context;
	vk::raii::Instance Instance{VK_NULL_HANDLE};
	// version 13 is minimum supported version
	uint32 ApiVersion = vk::ApiVersion13;

	std::vector<const char *> InstanceExtensions;
	std::vector<const char *> InstanceLayers;

	std::unique_ptr<FVulkanDevice> Device;

	FConfiguration Config = Default;

	void CreateInstance();
	void SelectDevice();

#if WIND_VULKAN_VALIDATION
	vk::raii::DebugUtilsMessengerEXT Messenger{VK_NULL_HANDLE};

	void SetupDebugCallbacks();
#endif
};
