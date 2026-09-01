#include "VulkanSwapchain.hpp"

#include "Check.hpp"
#include "Vulkan/Core/Private/Linux/VulkanLinux.hpp"
#include "VulkanDevice.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_to_string.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

// ok so we can't fully abstract presentation from core vulkan but we can put it in presentation layer file
// so it feels we have abstracted the presentation from core fully
void FVulkanDevice::SetupPresentQueue(vk::SurfaceKHR Surface)
{
	if (!PresentQueue)
	{
		const uint32 FamilyIndex = GetGraphicsQueue()->GetFamilyIndex();
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

FVulkanSwapChain *FVulkanSwapChain::Create(const vk::raii::Instance &InInstance, FVulkanDevice &InDevice,
                                           uint32 InWidth, uint32 InHeight, uint32 *DesiredImageCount,
                                           std::vector<vk::Image> &OutImages,
                                           FVulkanGenericPlatformWindowContext &WindowContext,
                                           FVulkanSwapchainRecreateInfo *RecreateInfo)
{

	// If Recreate Info is null it means this is first call to create so we create new surface
	// otherwise if recreate info is not null it means that there will be surface in
	// RecreateInfo so use that no need to create surface each time

	vk::SurfaceKHR Surface = VK_NULL_HANDLE;

	if (RecreateInfo != nullptr && RecreateInfo->SwapChain != nullptr)
	{
		CHECK(RecreateInfo->Surface != nullptr,
		      "Note there was a request to Recreate swapchain but `Surface` in RecreateInfo is null this is not a "
		      "normal behaviour in engine terminating...");
		Surface = RecreateInfo->Surface;
		RecreateInfo->Surface = VK_NULL_HANDLE;
	}
	else
	{
		// assuming the invariant that RecreteInfo == nullptr means first call to Create since we are in else
		// block we have to create a new surface let underlying platform create it for us
		FVulkanPlatform::CreateSurface(WindowContext, InInstance, &Surface);
	}

	VERIFYVULKANRESULT_UNWRAP(Formats, InDevice.GetPhysicalHandle().getSurfaceFormatsKHR());
	// TODO: for now do this later handle as surface lost
	CHECK(Formats.size() >= 1, "`VkGetSurfaceFormatsKHR` returned 0 formats");

	// TODO: for now hardcode much later when we will have console or config then read from there
	constexpr vk::SurfaceFormatKHR PreferredFormat = {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear};

	auto IsPrefferedFormatFound = std::ranges::find(Formats, PreferredFormat);

	vk::SurfaceFormatKHR SelectedFormat =
	    IsPrefferedFormatFound != Formats.end() ? *IsPrefferedFormatFound : Formats[0];

	// setup the presentation queue
	InDevice.SetupPresentQueue(Surface);

	VERIFYVULKANRESULT_UNWRAP(FoundPresentMode, InDevice.GetPhysicalHandle().getSurfacePresentModesKHR(Surface));

	// specs says VK_PRESENT_MODE_FIFO_KHR is always supported on any compliant implementation
	// but still for worst case we should have this check better than engine crashing without log
	CHECK(FoundPresentMode.size() >= 1, "Failed to find any presentation mode");

	// just hardcode this later when we have gui and console handle it through r.PresentMode
	// also as spec said FIFO is guarnteed to be available
	auto FifoPresentMode = std::ranges::find(FoundPresentMode, vk::PresentModeKHR::eFifo);

	VERIFYVULKANRESULT_UNWRAP(SurfaceCapabilities, InDevice.GetPhysicalHandle().getSurfaceCapabilitiesKHR(Surface));

	WIND_LOG(info, "Selected Present Mode: %s", vk::to_string(*FifoPresentMode));

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

	vk::SwapchainCreateInfoKHR SwapChainInfo{};
	SwapChainInfo.oldSwapchain = RecreateInfo->SwapChain != VK_NULL_HANDLE ? RecreateInfo->SwapChain : nullptr;
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
	    InDevice.GetHandle().createSwapchainKHR(SwapChainInfo);

	if (!SwapChainResult && SwapChainResult.error() != vk::Result::eSuccess)
	{
		FATAL("Create swapchain failed Error: %s", vk::to_string(SwapChainResult.error()));
	}

	WIND_LOG(info, "Creating swapchain with %s, %s, %s, images %d", vk::to_string(SwapChainInfo.presentMode),
	         vk::to_string(SwapChainInfo.imageFormat), vk::to_string(SwapChainInfo.imageColorSpace),
	         SwapChainInfo.minImageCount);

	vk::SwapchainKHR SwapChain = SwapChainResult.value();

	VERIFYVULKANRESULT_UNWRAP(SwapChainImages, InDevice.GetHandle().getSwapchainImagesKHR(SwapChain));
	OutImages = std::move(SwapChainImages);

	auto x = std::make_unique<FVulkanSwapChain>(
	    FVulkanSwapChain(InInstance, InDevice, Surface, SwapChain, InWidth, InWidth));

	return nullptr;
}
