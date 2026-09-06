#pragma once

#include "VulkanGenericPlatform.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"

#include <vector>


#include <cstddef>
#include <cstdint>


class FVulkanCore;
class FVulkanDevice;

// accept context in constructor
// provide create method takes dimesions and window context optionally recreate info creates swapchain and image
// internally something like
//  FVulkanSwapChain Swapchain{VulkanCore}; creates surface Swapchain->create();
// Swapchain->AcquireImage(idx);
// Swapchain->GetImageView(idx);
// Swapchain->Present(PresentInfo);
// Swapchain->Destroy();

struct FVulkanSwapchainRecreateInfo
{
	vk::SwapchainKHR SwapChain = VK_NULL_HANDLE;
	vk::SurfaceKHR Surface = VK_NULL_HANDLE;
};

class FVulkanSwapChain
{
  public:
	explicit FVulkanSwapChain(FVulkanCore &InCore);

	void Create(FVulkanGenericPlatformWindowContext &WindowContext, uint32_t InWidth, uint32_t InHeight,
	            uint32_t *DesiredImageCount, FVulkanSwapchainRecreateInfo *RecreateInfo);

	void Destroy(FVulkanSwapchainRecreateInfo *RecreateInfo);

  private:
	// TODO: later add current image index, present, window ID and all
	FVulkanCore &Core;
	vk::SurfaceKHR Surface = VK_NULL_HANDLE;
	vk::SwapchainKHR SwapChain = VK_NULL_HANDLE;
	std::vector<vk::Image> SwapChainImages;
	std::vector<vk::ImageView> SwapChainImageViews;

	uint32_t Width{};
	uint32_t Height{};
};
