#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <utility>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

enum class ETextureFormat : uint8_t
{
	BC7,
	BC5,
	BC4,
	BC1,

	Count
};

inline vk::Format ToVk(ETextureFormat Format)
{
	switch (Format)
	{
	case ETextureFormat::BC7:
		return vk::Format::eBc7SrgbBlock;
	case ETextureFormat::BC5:
		return vk::Format::eBc5UnormBlock;
	case ETextureFormat::BC4:
		return vk::Format::eBc4UnormBlock;
	case ETextureFormat::BC1:
		return vk::Format::eBc1RgbUnormBlock;

	default:
		std::unreachable();
	}
}

struct FVulkanTextureCreateInfo
{
	ETextureFormat Format;
	uint32_t Width;
	uint32_t Height;
	std::span<const std::byte> Pixels;
};

class FVulkanTexture
{
  public:
	VmaAllocation Allocation = VK_NULL_HANDLE;
	vk::Image Image = VK_NULL_HANDLE;
	vk::ImageView ImageView = VK_NULL_HANDLE;
	vk::Sampler Sampler = VK_NULL_HANDLE;
	vk::Format Format{};
	vk::Extent2D Extent{};

	FVulkanTexture(VmaAllocator InAllocator, vk::Device InDevice)
	    : Allocator(InAllocator), Allocation(VK_NULL_HANDLE), Device(InDevice), Image(VK_NULL_HANDLE),
	      ImageView(VK_NULL_HANDLE), Sampler(VK_NULL_HANDLE), Format(), Extent()
	{
	}

	FVulkanTexture(VmaAllocator InAllocator, VmaAllocation InAllocation, vk::Device InDevice, vk::Image InImage,
	               vk::ImageView InImageView, vk::Sampler InSampler, vk::Format InFormat, vk::Extent2D InExtent)
	    : Allocator(InAllocator), Allocation(InAllocation), Device(InDevice), Image(InImage), ImageView(InImageView),
	      Sampler(InSampler), Format(InFormat), Extent(InExtent)
	{
	}

	// some textures does not have sampler aka depth images
	FVulkanTexture(VmaAllocator InAllocator, VmaAllocation InAllocation, vk::Device InDevice, vk::Image InImage,
	               vk::ImageView InImageView, vk::Format InFormat, vk::Extent2D InExtent)
	    : Allocator(InAllocator), Allocation(InAllocation), Device(InDevice), Image(InImage), ImageView(InImageView),
	      Sampler(VK_NULL_HANDLE), Format(InFormat), Extent(InExtent)
	{
	}

	// Non-copyable
	FVulkanTexture(const FVulkanTexture &) = delete;
	FVulkanTexture &operator=(const FVulkanTexture &) = delete;

	FVulkanTexture(FVulkanTexture &&Other) noexcept
	    : Allocator(Other.Allocator), Allocation(std::exchange(Other.Allocation, VK_NULL_HANDLE)), Device(Other.Device),
	      Image(std::exchange(Other.Image, VK_NULL_HANDLE)), ImageView(std::exchange(Other.ImageView, VK_NULL_HANDLE)),
	      Sampler(std::exchange(Other.Sampler, VK_NULL_HANDLE)), Format(std::exchange(Other.Format, {})),
	      Extent(std::exchange(Other.Extent, {}))
	{
	}

	FVulkanTexture &operator=(FVulkanTexture &&Other) noexcept
	{
		if (this != &Other)
		{
			Destroy();

			Allocator = Other.Allocator;
			Allocation = std::exchange(Other.Allocation, VK_NULL_HANDLE);
			Device = Other.Device;

			Image = std::exchange(Other.Image, VK_NULL_HANDLE);
			ImageView = std::exchange(Other.ImageView, VK_NULL_HANDLE);
			Sampler = std::exchange(Other.Sampler, VK_NULL_HANDLE);
			Format = std::exchange(Other.Format, {});
			Extent = std::exchange(Other.Extent, {});
		};

		return *this;
	};

	~FVulkanTexture()
	{
		Destroy();
	}

  private:
	void Destroy() noexcept
	{
		if (ImageView != VK_NULL_HANDLE)
		{
			Device.destroyImageView(ImageView);
		}

		if (Sampler != VK_NULL_HANDLE)
		{
			Device.destroySampler(Sampler);
		}

		if (Image != VK_NULL_HANDLE)
		{
			vmaDestroyImage(Allocator, Image, Allocation);
		}

		Allocator = VK_NULL_HANDLE;
		Allocation = VK_NULL_HANDLE;

		Image = VK_NULL_HANDLE;
		ImageView = VK_NULL_HANDLE;
		Sampler = VK_NULL_HANDLE;

		Format = {};
		Extent = vk::Extent2D{};
	};

	VmaAllocator Allocator = VK_NULL_HANDLE;
	vk::Device Device = VK_NULL_HANDLE;
};
