#pragma once

#include <concepts>
#include <span>
#include <utility>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

class FVulkanQueue;
class FVulkanCommandBuffer;
class FVulkanFence;

enum class EBufferType : uint8_t
{
	Vertex,
	Index,
	Uniform
};

inline vk::BufferUsageFlags ToVk(EBufferType Type)
{
	switch (Type)
	{
	case EBufferType::Vertex:
		return vk::BufferUsageFlagBits::eVertexBuffer;
	case EBufferType::Index:
		return vk::BufferUsageFlagBits::eIndexBuffer;
	case EBufferType::Uniform:
		return vk::BufferUsageFlagBits::eUniformBuffer;

	default:
		std::unreachable();
	}
};

enum class ETextureFormat
{
	BC7,
	BC5,
	BC4,
	BC1
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

struct FVulkanBufferCreateInfo
{
	EBufferType Type;
	std::span<const std::byte> Data = {};
};

struct FVulkanBuffer
{
	VmaAllocator Allocator = VK_NULL_HANDLE;
	vk::Buffer Buffer = VK_NULL_HANDLE;
	VmaAllocation Allocation = VK_NULL_HANDLE;
	EBufferType Type = EBufferType::Vertex;
	void *MappedPtr = nullptr;

  public:
	FVulkanBuffer() = default;

	FVulkanBuffer(VmaAllocator InAllocator, vk::Buffer InBuffer, VmaAllocation InAllocation, EBufferType InType,
	              void *InMappedPtr = nullptr)
	    : Allocator(InAllocator), Buffer(InBuffer), Allocation(InAllocation), Type(InType), MappedPtr(InMappedPtr)
	{
	}

	FVulkanBuffer(VmaAllocator InAllocator, vk::Buffer InBuffer, VmaAllocation InAllocation,
	              void *InMappedPtr = nullptr)
	    : Allocator(InAllocator), Buffer(InBuffer), Allocation(InAllocation), Type(EBufferType::Vertex),
	      MappedPtr(InMappedPtr)
	{
	}

	FVulkanBuffer(const FVulkanBuffer &) = delete;
	FVulkanBuffer &operator=(const FVulkanBuffer &) = delete;

	// take ownership of Other's guts and leave Other empty
	FVulkanBuffer(FVulkanBuffer &&Other) noexcept
	    : Allocator(std::exchange(Other.Allocator, VK_NULL_HANDLE)),
	      Buffer(std::exchange(Other.Buffer, VK_NULL_HANDLE)),
	      Allocation(std::exchange(Other.Allocation, VK_NULL_HANDLE)), Type(std::exchange(Other.Type, {})),
	      MappedPtr(std::exchange(Other.MappedPtr, nullptr))
	{
	}

	FVulkanBuffer &operator=(FVulkanBuffer &&Other) noexcept
	{
		if (this != &Other)
		{
			Destroy();

			Allocator = std::exchange(Other.Allocator, VK_NULL_HANDLE);
			Buffer = std::exchange(Other.Buffer, VK_NULL_HANDLE);
			Allocation = std::exchange(Other.Allocation, VK_NULL_HANDLE);
			Type = std::exchange(Other.Type, EBufferType::Vertex);
			MappedPtr = std::exchange(Other.MappedPtr, nullptr);
		}

		return *this;
	};

	~FVulkanBuffer()
	{
		Destroy();
	}

  private:
	void Destroy() noexcept
	{
		if (Buffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(Allocator, Buffer, Allocation);

			Allocator = VK_NULL_HANDLE;
			Buffer = VK_NULL_HANDLE;
			Allocation = VK_NULL_HANDLE;
			Type = EBufferType::Vertex;
			MappedPtr = nullptr;
		}
	};
};

struct FVulkanTextureCreateInfo
{
	ETextureFormat Format;
	uint32_t Width;
	uint32_t Height;
	std::span<const std::byte> Pixels;
};

struct FVulkanTexture
{
  public:
	VmaAllocator Allocator = VK_NULL_HANDLE;
	VmaAllocation Allocation = VK_NULL_HANDLE;
	vk::Device Device = VK_NULL_HANDLE;

	vk::Image Image = VK_NULL_HANDLE;
	vk::ImageView ImageView = VK_NULL_HANDLE;
	vk::Sampler Sampler = VK_NULL_HANDLE;
	vk::Format Format;
	vk::Extent2D Extent{};

	FVulkanTexture() = default;

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
	    : Allocator(std::exchange(Other.Allocator, VK_NULL_HANDLE)),
	      Allocation(std::exchange(Other.Allocation, VK_NULL_HANDLE)),
	      Device(std::exchange(Other.Device, VK_NULL_HANDLE)), Image(std::exchange(Other.Image, VK_NULL_HANDLE)),
	      ImageView(std::exchange(Other.ImageView, VK_NULL_HANDLE)),
	      Sampler(std::exchange(Other.Sampler, VK_NULL_HANDLE)), Format(std::exchange(Other.Format, {})),
	      Extent(std::exchange(Other.Extent, {}))
	{
	}

	FVulkanTexture &operator=(FVulkanTexture &&Other) noexcept
	{
		if (this != &Other)
		{
			Destroy();

			Allocator = std::exchange(Other.Allocator, VK_NULL_HANDLE);
			Allocation = std::exchange(Other.Allocation, VK_NULL_HANDLE);
			Device = std::exchange(Other.Device, VK_NULL_HANDLE);

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
};

class FStagingAllocator
{
  public:
	FStagingAllocator() = default;
	FVulkanBuffer Upload(VmaAllocator Allocator, std::span<const std::byte> Data);
};

class FVulkanAllocator
{
  public:
	FVulkanAllocator(const vk::Instance Instance, const vk::PhysicalDevice Gpu, vk::Device InDevice,
	                 FVulkanQueue *InTransferQueue, FVulkanFence *InFence);

	FVulkanAllocator(const FVulkanAllocator &) = delete;
	FVulkanAllocator &operator=(const FVulkanAllocator &) = delete;

	FVulkanAllocator(FVulkanAllocator &&Other) noexcept
	    : Allocator(std::exchange(Other.Allocator, VK_NULL_HANDLE)),
	      StagingAllocator(std::move(Other.StagingAllocator)), Device(std::exchange(Other.Device, VK_NULL_HANDLE)),
	      TransferQueue(std::exchange(Other.TransferQueue, nullptr)),
	      CommandBuffer(std::exchange(Other.CommandBuffer, nullptr)), Fence(std::exchange(Other.Fence, nullptr)) {};

	FVulkanAllocator &operator=(FVulkanAllocator &&Other) = delete;
	~FVulkanAllocator();

	[[nodiscard]] std::vector<FVulkanBuffer> AllocateBuffers(std::span<const FVulkanBufferCreateInfo> BufferInfos);

	template <typename T>
	    requires std::same_as<std::remove_cvref_t<T>, FVulkanBufferCreateInfo>
	[[nodiscard]] std::vector<FVulkanBuffer> AllocateBuffers(T &&BufferInfo)
	{
		return AllocateBuffers(std::span<const FVulkanBufferCreateInfo>{std::addressof(BufferInfo), 1});
	}

	[[nodiscard]] std::vector<FVulkanTexture> AllocateTextures(std::span<const FVulkanTextureCreateInfo> TextureInfos);

	template <typename T>
	    requires std::same_as<std::remove_cvref_t<T>, FVulkanTextureCreateInfo>
	[[nodiscard]] std::vector<FVulkanTexture> AllocateTextures(T &&TextureInfo)
	{
		return AllocateTextures(std::span<const FVulkanTextureCreateInfo>{std::addressof(TextureInfo), 1});
	}

	inline VmaAllocator GetVmaAllocator() const
	{
		return Allocator;
	}

  private:
	FVulkanBuffer UploadToDeviceLocal(const FVulkanBufferCreateInfo &InBufferCreateInfo);

	VmaAllocator Allocator = VK_NULL_HANDLE;
	FStagingAllocator StagingAllocator;

	vk::Device Device = VK_NULL_HANDLE;

	// owned by FVulkanDevice
	FVulkanQueue *TransferQueue = nullptr;
	// owned by TransferQueue
	FVulkanCommandBuffer *CommandBuffer = nullptr;

	FVulkanFence *Fence = nullptr;
};
