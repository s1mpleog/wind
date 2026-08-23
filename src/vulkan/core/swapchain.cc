#include "swapchain.hpp"
#include "error.hpp"
#include "spdlog/spdlog.h"
#include "utils/expected_util.hpp"
#include "vulkan/core/configuration.hpp"
#include "vulkan/vulkan.hpp"
#include <algorithm>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace wind::vulkan::swapchain {
// presentation mode
// surface capabilities
// surface_format

WIND_NODISCARD static auto get_surface_capabilities(const vk::raii::SurfaceKHR&     surface,
                                                    const vk::raii::PhysicalDevice& physical_device) WIND_NOEXCEPT
    -> WindResult<vk::SurfaceCapabilitiesKHR>
{
  return WIND_TRY(physical_device.getSurfaceCapabilitiesKHR(surface));
}

WIND_NODISCARD static auto get_surface_format(const vk::raii::SurfaceKHR& surface, const vk::raii::PhysicalDevice& physical_device) WIND_NOEXCEPT
    -> WindResult<vk::SurfaceFormatKHR>
{
  auto available_formats = WIND_TRY(physical_device.getSurfaceFormatsKHR(surface));
  WIND_ENSURE_NOT_EMPTY(available_formats, WindError::vulkan(ErrorCode::FailedToGetSurfaceFormats));

  constexpr vk::SurfaceFormatKHR preferred{vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear};

  auto it = std::ranges::find(available_formats, preferred);

  if(it != available_formats.end())
    return *it;

  return available_formats.front();
}

WIND_NODISCARD static auto get_presentation_mode(const Configuration&            cfg,
                                                 const vk::raii::SurfaceKHR&     surface,
                                                 const vk::raii::PhysicalDevice& physical_device) WIND_NOEXCEPT
    -> WindResult<vk::PresentModeKHR>
{
  auto available_modes = WIND_TRY(physical_device.getSurfacePresentModesKHR(surface));
  WIND_ENSURE_NOT_EMPTY(available_modes, WindError::vulkan(ErrorCode::FailedToGetSurfacePresentModes));

  // if cfg.vsync is false then just return immediate_mode or if vsync is enable then return fifo mode
  // or if cfg.vsync and cfg.triple_buffering then return mailbox mode

  auto supported = [&](vk::PresentModeKHR mode) -> bool {
    return std::ranges::find(available_modes, mode) != available_modes.end();
  };

  auto preferred_mode = [&]() -> vk::PresentModeKHR {
    if(!cfg.vsync)
      return supported(vk::PresentModeKHR::eImmediate) ? vk::PresentModeKHR::eImmediate : vk::PresentModeKHR::eFifo;

    if(cfg.buffering == Buffering::TripleBuffering)
      return supported(vk::PresentModeKHR::eMailbox) ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;

    return vk::PresentModeKHR::eFifo;
  }();

  return preferred_mode;
}

WIND_NODISCARD static auto get_images(const vk::raii::SwapchainKHR& swapchain) WIND_NOEXCEPT -> WindResult<std::vector<vk::Image>>
{
  return WIND_TRY(swapchain.getImages());
}

WIND_NODISCARD static auto create_image_views(std::span<const vk::Image> images, const vk::raii::Device& device, vk::Format format) WIND_NOEXCEPT
    -> WindResult<std::vector<vk::raii::ImageView>>
{
  std::vector<vk::raii::ImageView> image_views;
  image_views.reserve(images.size());

  for(const auto& image : images)
  {
    vk::ImageViewCreateInfo create_info{};
    create_info.image                           = image;
    create_info.format                          = format;
    create_info.viewType                        = vk::ImageViewType::e2D;
    create_info.subresourceRange.levelCount     = 1;
    create_info.subresourceRange.layerCount     = 1;
    create_info.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
    create_info.subresourceRange.baseMipLevel   = 0;
    create_info.subresourceRange.baseArrayLayer = 0;

    image_views.emplace_back(WIND_TRY(device.createImageView(create_info)));
  }

  return image_views;
}

WIND_NODISCARD
auto create(const Configuration&          cfg,
            u32                           window_width,
            u32                           window_height,
            const vk::raii::SurfaceKHR&   surface,
            const DeviceContext&          device_context,
            const vk::raii::SwapchainKHR* old_swapchain) WIND_NOEXCEPT -> WindResult<SwapchainContext>
{
  auto surface_capabilities = WIND_TRY(get_surface_capabilities(surface, device_context.physical_device));
  auto surface_format       = WIND_TRY(get_surface_format(surface, device_context.physical_device));
  auto presentation_mode    = WIND_TRY(get_presentation_mode(cfg, surface, device_context.physical_device));

  vk::SwapchainCreateInfoKHR create_info{};

  create_info.presentMode      = presentation_mode;
  create_info.surface          = surface;
  create_info.clipped          = vk::True;
  create_info.compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  create_info.preTransform     = surface_capabilities.currentTransform;
  create_info.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment;
  create_info.imageArrayLayers = 1;
  create_info.imageFormat      = surface_format.format;
  create_info.imageColorSpace  = surface_format.colorSpace;

  u32 image_count = surface_capabilities.minImageCount + 1;

  if(surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount)
    image_count = surface_capabilities.maxImageCount;

  create_info.minImageCount = image_count;

  vk::Extent2D extent{};

  if(surface_capabilities.currentExtent.width != UINT32_MAX)
  {
    extent = surface_capabilities.currentExtent;
  }
  else
  {
    extent.width =
        std::clamp(window_width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);

    extent.height =
        std::clamp(window_height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);
  }

  create_info.imageExtent = extent;

  const std::array queues_index = {device_context.graphics_queue_idx.value(), device_context.presentation_queue_idx.value()};

  if(device_context.graphics_queue_idx != device_context.presentation_queue_idx)
  {
    create_info.imageSharingMode      = vk::SharingMode::eConcurrent;
    create_info.queueFamilyIndexCount = queues_index.size();
    create_info.pQueueFamilyIndices   = queues_index.data();
  }
  else
  {
    create_info.imageSharingMode      = vk::SharingMode::eExclusive;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices   = nullptr;
  }

  create_info.oldSwapchain = (old_swapchain != nullptr) ? **old_swapchain : vk::SwapchainKHR{};

  auto swapchain   = WIND_TRY(device_context.handle.createSwapchainKHR(create_info));
  auto images      = WIND_TRY(get_images(swapchain));
  auto image_views = WIND_TRY(create_image_views(images, device_context.handle, surface_format.format));

#ifdef WIND_LOG_ENABLE
  spdlog::info("Swapchain created dimension: {}x{}, images: {}", extent.width, extent.height, images.size());
#endif

  return SwapchainContext{.handle      = std::move(swapchain),
                          .images      = std::move(images),
                          .image_views = std::move(image_views),
                          .format      = surface_format,
                          .extent      = extent};
}

};  // namespace wind::vulkan::swapchain
