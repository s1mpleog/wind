#pragma once

#include "Config.hpp"

#include <inplace_vector>

class FVulkanSwapChain;
class FVulkanDevice;
struct FVulkanSwapchainRecreateInfo;

class FVulkanViewport
{
  public:
	FVulkanViewport(FVulkanDevice &InDevice, void *InWindowHandle, uint32 InSizeX, uint32 InSizeY);

	void CreateSwapchain();

	WIND_INLINE FVulkanSwapChain *GetSwapChain()
	{
		return SwapChain;
	}

	WIND_INLINE void *GetWindowHandle()
	{
		return WindowHandle;
	}

	void DestroySwapChain(FVulkanSwapchainRecreateInfo *RecreateInfo);

	constexpr static uint32 NumRequestedSwapChainImages = 3;

  private:
	void InitImages();

	FVulkanDevice &Device;

	void *WindowHandle = nullptr;

	uint32 SizeX{};
	uint32 SizeY{};
	FVulkanSwapChain *SwapChain;
	std::inplace_vector<vk::Image, NumRequestedSwapChainImages> Images;
};
