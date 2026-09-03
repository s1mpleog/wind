#pragma once

// let's think this through this eh?
// it needs to handle surface and swapchain think of this class as conceptually bridge between vulkan and presentation
// it's job is to create a swapchain and surface manage recreation and swapchain image and image views
// optionally it should also transition the swapchain images from undefined to color_attchment_optional
// we also did not created presentation queue or fetched it so we have to handle that also
// does surface needs to be created each time no then i have to handle that also
// ownership would be that  FVulkanSwapChain needs FVulkanDevice for swapchain creation and all
// it also needs FWindowContext and vk::Instance so it can create surface if needed

#include "Config.hpp"
#include "Vulkan/Core/Private/VulkanGenericPlatform.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_raii.hpp"

#include <vector>

class FVulkanDevice;

struct FVulkanSwapchainRecreateInfo
{
	vk::SwapchainKHR SwapChain = VK_NULL_HANDLE;
	vk::SurfaceKHR Surface = VK_NULL_HANDLE;
};

class FVulkanSwapChain
{
	FVulkanSwapChain(const vk::raii::Instance &InInstance, FVulkanDevice &InDevice, vk::SurfaceKHR &InSurface,
	                 vk::SwapchainKHR InSwapChain, uint32 InWidth, uint32 InHeight);

  public:
	WIND_NODISCARD static std::unique_ptr<FVulkanSwapChain>
	Create(const vk::raii::Instance &InInstance, FVulkanDevice &Device, uint32 InWidth, uint32 InHeight,
	       uint32 *DesiredImageCount, std::vector<vk::Image> &OutImages,
	       FVulkanGenericPlatformWindowContext &WindowContext, FVulkanSwapchainRecreateInfo *RecreateInfo);

	void Destroy(FVulkanSwapchainRecreateInfo *RecreateInfo);

  private:
	// TODO: later add current image index, present, window ID and all
	const vk::raii::Instance &Instance = VK_NULL_HANDLE;
	FVulkanDevice &Device;
	vk::SwapchainKHR SwapChain = VK_NULL_HANDLE;
	vk::SurfaceKHR Surface = VK_NULL_HANDLE;

	uint32 Width{};
	uint32 Height{};
};
