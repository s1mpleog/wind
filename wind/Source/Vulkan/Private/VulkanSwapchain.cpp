
#include "VulkanSwapchain.hpp"

#include "Check.hpp"
// TODO: better fix for this
#if defined(_WIN32)
#include "Windows/VulkanWindows.hpp"
#elif defined(__linux__)
#include "Linux/VulkanLinux.hpp"
#endif
#include "VulkanCheck.hpp"
#include "VulkanCore.hpp"
#include "VulkanDevice.h"
#include "VulkanGenericPlatform.h"
#include "spdlog/spdlog.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_to_string.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

// ok so we can't fully abstract presentation from core vulkan but we can put it in presentation layer file
// so it feels we have abstracted the presentation from core fully
void FVulkanDevice::SetupPresentQueue(vk::SurfaceKHR Surface)
{
	if (!PresentQueue)
	{
		const uint32_t FamilyIndex = GetGraphicsQueue()->GetFamilyIndex();
		vk::Bool32 SupportsPresent = vk::False;
		VERIFYVULKANRESULT(Gpu.getSurfaceSupportKHR(FamilyIndex, Surface, &SupportsPresent));

		if (!SupportsPresent)
		{
			FATAL("Note Engine needs a Presentation queue to display stuffs Vulkan did not found any queue in selected "
			      "GPU that supports presentation queue terminating...");
		}

		PresentQueue = GetGraphicsQueue();
	}
};

FVulkanSwapChain::FVulkanSwapChain(FVulkanCore &InCore) : Core(InCore) {};

void FVulkanSwapChain::Create(FVulkanGenericPlatformWindowContext &WindowContext, uint32_t InWidth, uint32_t InHeight,
                              uint32_t *DesiredImageCount, FVulkanSwapchainRecreateInfo *RecreateInfo)
{
	// if recreate info is nullptr then assume this is first call
	// else assume this is recreate info call
	// if its first call then create surface
	// otherwise use existing surface

	if (RecreateInfo != nullptr && RecreateInfo->SwapChain != nullptr)
	{
		CHECK(RecreateInfo->Surface != nullptr,
		      "Note there was a request to Recreate swapchain but `Surface` in RecreateInfo is null this is not a "
		      "normal behaviour in engine terminating...");

		for (vk::ImageView &ImageView : SwapChainImageViews)
		{
			Core.GetDevice()->GetHandle().destroyImageView(ImageView);
		}

		SwapChainImageViews.clear();

		Surface = RecreateInfo->Surface;
		RecreateInfo->Surface = VK_NULL_HANDLE;
	}
	else
	{
		FVulkanPlatform::CreateSurface(WindowContext, Core.GetInstance(), &Surface);
	}

	vk::PhysicalDevice Gpu = Core.GetDevice()->GetPhysicalHandle();

	VERIFYVULKANRESULT_UNWRAP(Formats, Gpu.getSurfaceFormatsKHR(Surface));
	// TODO: for now do this later handle as surface lost
	CHECK(Formats.size() >= 1, "`VkGetSurfaceFormatsKHR` returned 0 formats");

	// TODO: for now hardcode much later when we will have console or config then read from there
	constexpr vk::SurfaceFormatKHR PreferredFormat = {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear};

	auto IsPrefferedFormatFound = std::ranges::find(Formats, PreferredFormat);

	vk::SurfaceFormatKHR SelectedFormat =
	    IsPrefferedFormatFound != Formats.end() ? *IsPrefferedFormatFound : Formats[0];

	// setup the presentation queue
	Core.GetDevice()->SetupPresentQueue(Surface);

	VERIFYVULKANRESULT_UNWRAP(FoundPresentMode, Gpu.getSurfacePresentModesKHR(Surface));

	// specs says VK_PRESENT_MODE_FIFO_KHR is always supported on any compliant implementation
	// but still for worst case we should have this check better than engine crashing without log
	CHECK(FoundPresentMode.size() >= 1, "Failed to find any presentation mode");

	// just hardcode this later when we have gui and console handle it through r.PresentMode
	// also as spec said FIFO is guarnteed to be available
	auto FifoPresentMode = std::ranges::find(FoundPresentMode, vk::PresentModeKHR::eFifo);

	VERIFYVULKANRESULT_UNWRAP(SurfaceCapabilities, Gpu.getSurfaceCapabilitiesKHR(Surface));

	WIND_LOG(info, "Selected Present Mode: {}", vk::to_string(*FifoPresentMode));

	if (SurfaceCapabilities.maxImageCount > 0 && *DesiredImageCount > SurfaceCapabilities.maxImageCount)
	{
		*DesiredImageCount = SurfaceCapabilities.maxImageCount;
	}

	// in wayland for some reasons we don't usually get valid surface extent
	bool bIsValidSurfaceExtent = SurfaceCapabilities.currentExtent.width != UINT32_MAX;

	if (!bIsValidSurfaceExtent)
	{
		SurfaceCapabilities.currentExtent.width =
		    std::clamp(InWidth, SurfaceCapabilities.minImageExtent.width, SurfaceCapabilities.maxImageExtent.width);

		SurfaceCapabilities.currentExtent.height =
		    std::clamp(InHeight, SurfaceCapabilities.minImageExtent.height, SurfaceCapabilities.maxImageExtent.height);
	}

	vk::SwapchainPresentModesCreateInfoKHR PresentModesInfo{};
	PresentModesInfo.presentModeCount = 1;
	PresentModesInfo.pPresentModes = &*FifoPresentMode;

	vk::SwapchainCreateInfoKHR SwapChainInfo{};
	SwapChainInfo.pNext = &PresentModesInfo;

	SwapChainInfo.oldSwapchain = nullptr;

	vk::SwapchainKHR OldSwapchain = VK_NULL_HANDLE;

	if (RecreateInfo && RecreateInfo->SwapChain != VK_NULL_HANDLE)
	{
		OldSwapchain = RecreateInfo->SwapChain;
		SwapChainInfo.oldSwapchain = OldSwapchain;
	}

	SwapChainInfo.clipped = vk::True;
	SwapChainInfo.imageFormat = SelectedFormat.format;
	SwapChainInfo.imageColorSpace = SelectedFormat.colorSpace;
	SwapChainInfo.imageExtent = SurfaceCapabilities.currentExtent;
	SwapChainInfo.surface = Surface;
	SwapChainInfo.minImageCount = *DesiredImageCount;
	SwapChainInfo.presentMode = *FifoPresentMode;
	SwapChainInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
	SwapChainInfo.preTransform = SurfaceCapabilities.currentTransform;
	SwapChainInfo.imageArrayLayers = 1;
	// todo: don't hardcode this
	SwapChainInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	// since we are using eExclusive mode we don't have to pass queue index into pQueueFamilyIndices
	SwapChainInfo.imageSharingMode = vk::SharingMode::eExclusive;

	vk::ResultValueType<vk::SwapchainKHR>::type SwapChainResult =
	    Core.GetDevice()->GetHandle().createSwapchainKHR(SwapChainInfo);

	if (!SwapChainResult && SwapChainResult.error() != vk::Result::eSuccess)
	{
		FATAL("Create swapchain failed Error: {}", vk::to_string(SwapChainResult.error()));
	}

	WIND_LOG(info, "Creating swapchain with {}, {}, {}, {}, {}, images {}", SwapChainInfo.imageExtent.width,
	         SwapChainInfo.imageExtent.height, vk::to_string(SwapChainInfo.presentMode),
	         vk::to_string(SwapChainInfo.imageFormat), vk::to_string(SwapChainInfo.imageColorSpace),
	         SwapChainInfo.minImageCount);

	SwapChain = SwapChainResult.value();

	// destroy old swapchain
	if (OldSwapchain != VK_NULL_HANDLE)
	{
		// todo: find a better way later instead of wait until idle
		Core.GetDevice()->WaitUntilIdle();
		Core.GetDevice()->GetHandle().destroySwapchainKHR(OldSwapchain);
	}

	Extent = SurfaceCapabilities.currentExtent;
	Format = SwapChainInfo.imageFormat;

	VERIFYVULKANRESULT_UNWRAP(TempSwapChainImages, Core.GetDevice()->GetHandle().getSwapchainImagesKHR(SwapChain));

	SwapChainImages = std::move(TempSwapChainImages);

	SwapChainImageViews.reserve(SwapChainImages.size());

	for (const vk::Image &Image : SwapChainImages)
	{
		vk::ImageViewCreateInfo ImageViewInfo{};
		ImageViewInfo.image = Image;
		ImageViewInfo.viewType = vk::ImageViewType::e2D;
		ImageViewInfo.format = SwapChainInfo.imageFormat;
		ImageViewInfo.setSubresourceRange(vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

		VERIFYVULKANRESULT_UNWRAP(ImageView, Core.GetDevice()->GetHandle().createImageView(ImageViewInfo));

		SwapChainImageViews.push_back(std::move(ImageView));
	}
}

const vk::Image FVulkanSwapChain::GetImage(uint32_t Index) const
{
	return SwapChainImages[Index];
}
const vk::ImageView FVulkanSwapChain::GetImageView(uint32_t Index) const
{
	return SwapChainImageViews[Index];
}

void FVulkanSwapChain::Destroy(FVulkanSwapchainRecreateInfo *RecreateInfo)
{
	// wait for device to be idle before doing
	// if we have recreated info then assign this->SwapChain and this->Surface to RecreateInfo
	// otherwise destroy swapchain and surface

	Core.GetDevice()->WaitUntilIdle();

	for (vk::ImageView &ImageView : SwapChainImageViews)
	{
		Core.GetDevice()->GetHandle().destroyImageView(ImageView);
	}

	SwapChainImageViews.clear();

	if (SwapChain != VK_NULL_HANDLE)
	{
		Core.GetDevice()->GetHandle().destroySwapchainKHR(SwapChain);
		SwapChain = VK_NULL_HANDLE;
	}

	if (Surface != VK_NULL_HANDLE)
	{
		FVulkanPlatform::DestroySurface(Core.GetInstance(), Surface);
		Surface = VK_NULL_HANDLE;
	}

	spdlog::info("destroyed swapchain, surface and image views");
}
