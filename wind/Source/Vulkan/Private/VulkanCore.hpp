#pragma once

#include "Configuration.hpp"
#include "vulkan/vulkan_profiles.hpp"

#include <cstdint>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

class FVulkanDevice;

class FVulkanCore
{
  public:
	void Initialize() noexcept;

	FVulkanCore(FConfiguration &InConfig);
	~FVulkanCore();

	FVulkanCore Get()
	{
		return *this;
	}

	// TODO: maybe prefer span
	const std::vector<const char *> &GetInstanceExtensions() const noexcept
	{
		return InstanceExtensions;
	};

	const std::vector<const char *> &GetInstanceLayers() const noexcept
	{
		return InstanceLayers;
	};

	const vk::Instance &GetInstance() const noexcept
	{
		return Instance;
	}

	FVulkanDevice *GetDevice() noexcept
	{
		return Device;
	}

	uint32_t GetApiVersion() const
	{
		return ApiVersion;
	}

	// later set image layout, get viewports

	static constexpr VpProfileProperties ProfileProperties = {VP_KHR_ROADMAP_2022_NAME,
	                                                          VP_KHR_ROADMAP_2022_SPEC_VERSION};

  private:
	vk::Instance Instance{VK_NULL_HANDLE};

	uint32_t ApiVersion = vk::ApiVersion13;

	std::vector<const char *> InstanceExtensions;
	std::vector<const char *> InstanceLayers;

	FVulkanDevice *Device = nullptr;

	FConfiguration Config = Default;

	void CreateInstance();
	void SelectDevice();

#if WIND_VULKAN_VALIDATION
	vk::DebugUtilsMessengerEXT Messenger{VK_NULL_HANDLE};
	void SetupDebugCallbacks();
	void RemoveDebugCallbacks();
#endif
};
