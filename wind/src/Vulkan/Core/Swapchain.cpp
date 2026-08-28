#include "Vulkan/Core/Swapchain.hpp"

#include "Error.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Configuration.hpp"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <span>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

// presentation mode
// surface capabilities
// surface_format

WIND_NODISCARD static auto GetSurfaceCapabilities(const vk::raii::SurfaceKHR &Surface,
                                                  const vk::raii::PhysicalDevice &PhysicalDevice) WIND_NOEXCEPT
    -> WindResult<vk::SurfaceCapabilitiesKHR>
{
	return WIND_TRY(PhysicalDevice.getSurfaceCapabilitiesKHR(Surface));
}

WIND_NODISCARD static auto GetSurfaceFormat(const vk::raii::SurfaceKHR &Surface,
                                            const vk::raii::PhysicalDevice &PhysicalDevice) WIND_NOEXCEPT
    -> WindResult<vk::SurfaceFormatKHR>
{
	auto AvailableFormats = WIND_TRY(PhysicalDevice.getSurfaceFormatsKHR(Surface));
	WIND_ENSURE_NOT_EMPTY(AvailableFormats, WindError::vulkan(ErrorCode::FailedToGetSurfaceFormats));

	constexpr vk::SurfaceFormatKHR Preferred{vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear};

	auto It = std::ranges::find(AvailableFormats, Preferred);

	if (It != AvailableFormats.end())
		return *It;

	return AvailableFormats.front();
}

WIND_NODISCARD static auto GetPresentationMode(const FConfiguration &Cfg, const vk::raii::SurfaceKHR &Surface,
                                               const vk::raii::PhysicalDevice &PhysicalDevice) WIND_NOEXCEPT
    -> WindResult<vk::PresentModeKHR>
{
	auto AvailableModes = WIND_TRY(PhysicalDevice.getSurfacePresentModesKHR(Surface));
	WIND_ENSURE_NOT_EMPTY(AvailableModes, WindError::vulkan(ErrorCode::FailedToGetSurfacePresentModes));

	// if cfg.vsync is false then just return immediate_mode or if vsync is enable then return fifo mode
	// or if cfg.vsync and cfg.triple_buffering then return mailbox mode

	auto Supported = [&](vk::PresentModeKHR Mode) -> bool
	{ return std::ranges::find(AvailableModes, Mode) != AvailableModes.end(); };

	auto PreferredMode = [&]() -> vk::PresentModeKHR
	{
		if (!Cfg.Vsync)
			return Supported(vk::PresentModeKHR::eImmediate) ? vk::PresentModeKHR::eImmediate
			                                                 : vk::PresentModeKHR::eFifo;

		if (Cfg.Buffering == EBuffering::TripleBuffering)
			return Supported(vk::PresentModeKHR::eMailbox) ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;

		return vk::PresentModeKHR::eFifo;
	}();

	return PreferredMode;
}

WIND_NODISCARD static auto GetImages(const vk::raii::SwapchainKHR &Swapchain) WIND_NOEXCEPT
    -> WindResult<std::vector<vk::Image>>
{
	return WIND_TRY(Swapchain.getImages());
}

WIND_NODISCARD static auto CreateImageViews(std::span<const vk::Image> Images, const vk::raii::Device &Device,
                                            vk::Format Format) WIND_NOEXCEPT
    -> WindResult<std::vector<vk::raii::ImageView>>
{
	std::vector<vk::raii::ImageView> ImageViews;
	ImageViews.reserve(Images.size());

	for (const auto &Image : Images)
	{
		vk::ImageViewCreateInfo CreateInfo{};
		CreateInfo.image = Image;
		CreateInfo.format = Format;
		CreateInfo.viewType = vk::ImageViewType::e2D;
		CreateInfo.subresourceRange.levelCount = 1;
		CreateInfo.subresourceRange.layerCount = 1;
		CreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		CreateInfo.subresourceRange.baseMipLevel = 0;
		CreateInfo.subresourceRange.baseArrayLayer = 0;

		ImageViews.emplace_back(WIND_TRY(Device.createImageView(CreateInfo)));
	}

	return ImageViews;
}

WIND_NODISCARD
auto CreateSwapchain(const FConfiguration &Cfg, u32 WindowWidth, u32 WindowHeight, const vk::raii::SurfaceKHR &Surface,
                     const FGpuDevice &DeviceContext, const vk::raii::SwapchainKHR *OldSwapchain) WIND_NOEXCEPT
    -> WindResult<FSwapchainContext>
{
	auto SurfaceCapabilities = WIND_TRY(GetSurfaceCapabilities(Surface, DeviceContext.PhysicalDevice));
	auto SurfaceFormat = WIND_TRY(GetSurfaceFormat(Surface, DeviceContext.PhysicalDevice));
	auto PresentationMode = WIND_TRY(GetPresentationMode(Cfg, Surface, DeviceContext.PhysicalDevice));

	spdlog::info("USING {} PRESENTATION MODE", vk::to_string(PresentationMode));

	vk::SwapchainCreateInfoKHR CreateInfo{};

	CreateInfo.presentMode = PresentationMode;
	CreateInfo.surface = Surface;
	CreateInfo.clipped = vk::True;
	CreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	CreateInfo.preTransform = SurfaceCapabilities.currentTransform;
	CreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
	CreateInfo.imageArrayLayers = 1;
	CreateInfo.imageFormat = SurfaceFormat.format;
	CreateInfo.imageColorSpace = SurfaceFormat.colorSpace;

	u32 ImageCount = SurfaceCapabilities.minImageCount + 1;

	if (SurfaceCapabilities.maxImageCount > 0 && ImageCount > SurfaceCapabilities.maxImageCount)
		ImageCount = SurfaceCapabilities.maxImageCount;

	CreateInfo.minImageCount = ImageCount;

	vk::Extent2D Extent{};

	if (SurfaceCapabilities.currentExtent.width != UINT32_MAX)
	{
		Extent = SurfaceCapabilities.currentExtent;
	}
	else
	{
		Extent.width =
		    std::clamp(WindowWidth, SurfaceCapabilities.minImageExtent.width, SurfaceCapabilities.maxImageExtent.width);

		Extent.height = std::clamp(WindowHeight, SurfaceCapabilities.minImageExtent.height,
		                           SurfaceCapabilities.maxImageExtent.height);
	}

	CreateInfo.imageExtent = Extent;

	const std::array QueuesIndex = {DeviceContext.GraphicsQueueIdx.value(), DeviceContext.PresentationQueueIdx.value()};

	if (DeviceContext.GraphicsQueueIdx != DeviceContext.PresentationQueueIdx)
	{
		CreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		CreateInfo.queueFamilyIndexCount = QueuesIndex.size();
		CreateInfo.pQueueFamilyIndices = QueuesIndex.data();
	}
	else
	{
		CreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
		CreateInfo.queueFamilyIndexCount = 0;
		CreateInfo.pQueueFamilyIndices = nullptr;
	}

	CreateInfo.oldSwapchain = (OldSwapchain != nullptr) ? **OldSwapchain : vk::SwapchainKHR{};

	auto Swapchain = WIND_TRY(DeviceContext.Device.createSwapchainKHR(CreateInfo));

	auto Images = WIND_TRY(GetImages(Swapchain));
	auto ImageViews = WIND_TRY(CreateImageViews(Images, DeviceContext.Device, SurfaceFormat.format));

#ifdef WIND_LOG_ENABLE
	spdlog::info("Swapchain created dimension: {}x{}, images: {}", Extent.width, Extent.height, Images.size());
#endif

	return FSwapchainContext{.Handle = std::move(Swapchain),
	                         .Images = std::move(Images),
	                         .ImageViews = std::move(ImageViews),
	                         .Format = SurfaceFormat,
	                         .Extent = Extent};
}
