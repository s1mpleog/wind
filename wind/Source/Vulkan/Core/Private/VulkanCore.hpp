#pragma once

#include "Vulkan/Core/Configuration.hpp"
#include "vulkan/vulkan_profiles.hpp"

#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

class FVulkanDevice;

class FVulkanCore
{
  public:
	void Initialize() WIND_NOEXCEPT;

	FVulkanCore(FConfiguration &InConfig);
	~FVulkanCore();

	// TODO: maybe prefer span
	const std::vector<const char *> &GetInstanceExtensions() const WIND_NOEXCEPT
	{
		return InstanceExtensions;
	};

	const std::vector<const char *> &GetInstanceLayers() const WIND_NOEXCEPT
	{
		return InstanceLayers;
	};

	const vk::Instance &GetInstance() const WIND_NOEXCEPT
	{
		return Instance;
	}

	FVulkanDevice *GetDevice() WIND_NOEXCEPT
	{
		return Device.get();
	}

	uint32 GetApiVersion() const
	{
		return ApiVersion;
	}

	// later set image layout, get viewports

	static constexpr VpProfileProperties ProfileProperties = {VP_KHR_ROADMAP_2022_NAME,
	                                                          VP_KHR_ROADMAP_2022_SPEC_VERSION};

  private:
	vk::Instance Instance{VK_NULL_HANDLE};
	uint32 ApiVersion = vk::ApiVersion13;

	std::vector<const char *> InstanceExtensions;
	std::vector<const char *> InstanceLayers;

	std::unique_ptr<FVulkanDevice> Device;

	FConfiguration Config = Default;

	void CreateInstance();
	void SelectDevice();

#if WIND_VULKAN_VALIDATION
	vk::DebugUtilsMessengerEXT Messenger{VK_NULL_HANDLE};

	void SetupDebugCallbacks();
#endif
};
