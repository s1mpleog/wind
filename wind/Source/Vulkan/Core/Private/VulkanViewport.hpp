#pragma once

#include <inplace_vector>

class FVulkanSwapChain;
class FVulkanDevice;

class FVulkanViewport
{
  public:
	FVulkanViewport(FVulkanDevice &InDevice, void *InWindowHandle, uint32 InSizeX, uint32 InSizeY);

	constexpr static uint32 NumRequestedSwapChainImages = 3;

	void CreateSwapchain();

  private:
	void InitImages();

	FVulkanDevice &Device;

	void *WindowHandle = nullptr;

	uint32 SizeX{};
	uint32 SizeY{};
	FVulkanSwapChain *SwapChain;
	std::inplace_vector<vk::Image, NumRequestedSwapChainImages> Images;
};
