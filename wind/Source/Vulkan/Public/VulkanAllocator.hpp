#pragma once

#include <span>
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

enum class EMemoryType : uint8_t
{
	DeviceLocal,
	HostVisible,
	HostCoherent,
};

struct FVulkanBufferCreateInfo
{
	EBufferType Type;
	std::span<const std::byte> Data = {};
};

struct FVulkanBuffer
{
	vk::Buffer Buffer;
	VmaAllocation Allocation;
	void *MappedPtr = nullptr;
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
	FVulkanAllocator() = default;
	FVulkanAllocator(const vk::Instance Instance, const vk::PhysicalDevice Gpu, const vk::Device Device);
	~FVulkanAllocator();

	FVulkanBuffer AllocateBuffer(const FVulkanBufferCreateInfo &BufferInfo, std::span<const std::byte> Data = {});
	FVulkanBuffer AllocateBuffer(std::span<const FVulkanBufferCreateInfo> BufferInfos);

  private:
	FVulkanBuffer UploadToDeviceLocal(const FVulkanBufferCreateInfo &InBufferCreateInfo);

  private:
	VmaAllocator Allocator = VK_NULL_HANDLE;
	FStagingAllocator StagingAllocator;
};
