#pragma once

#include <cstdint>
#include <span>
#include <utility>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

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
