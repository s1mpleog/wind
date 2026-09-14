#pragma once

#include "VulkanAllocationError.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanTexture.hpp"

#include <concepts>
#include <expected>
#include <span>
#include <utility>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

class FVulkanQueue;
class FVulkanCommandBuffer;
class FVulkanFence;

class FStagingAllocator
{
  public:
	TAllocationResult<FVulkanBuffer> Upload(VmaAllocator Allocator, std::span<const std::byte> Data);
};

class FVulkanAllocator
{
  public:
	FVulkanAllocator(const vk::Instance Instance, const vk::PhysicalDevice InGpu, vk::Device InDevice,
	                 FVulkanQueue *InTransferQueue, FVulkanFence *InFence, bool bInHostImageCopySupported);

	FVulkanAllocator(const FVulkanAllocator &) = delete;
	FVulkanAllocator &operator=(const FVulkanAllocator &) = delete;

	FVulkanAllocator(FVulkanAllocator &&Other) noexcept
	    : Allocator(std::exchange(Other.Allocator, VK_NULL_HANDLE)),
	      StagingAllocator(std::move(Other.StagingAllocator)), Gpu(std::exchange(Other.Gpu, VK_NULL_HANDLE)),
	      Device(std::exchange(Other.Device, VK_NULL_HANDLE)),
	      TransferQueue(std::exchange(Other.TransferQueue, nullptr)),
	      CommandBuffer(std::exchange(Other.CommandBuffer, nullptr)), Fence(std::exchange(Other.Fence, nullptr)),
	      bHostImageCopySupported(std::exchange(Other.bHostImageCopySupported, false)) {};

	FVulkanAllocator &operator=(FVulkanAllocator &&Other) = delete;
	~FVulkanAllocator();

	[[nodiscard]] TAllocationResult<std::vector<FVulkanBuffer>>
	AllocateBuffers(std::span<const FVulkanBufferCreateInfo> BufferInfos);

	template <typename T>
	    requires std::same_as<std::remove_cvref_t<T>, FVulkanBufferCreateInfo>
	[[nodiscard]] TAllocationResult<std::vector<FVulkanBuffer>> AllocateBuffers(T &&BufferInfo)
	{
		return AllocateBuffers(std::span<const FVulkanBufferCreateInfo>{std::addressof(BufferInfo), 1});
	}

	[[nodiscard]] TAllocationResult<std::vector<FVulkanTexture>>
	AllocateTextures(std::span<const FVulkanTextureCreateInfo> TextureInfos);

	template <typename T>
	    requires std::same_as<std::remove_cvref_t<T>, FVulkanTextureCreateInfo>
	[[nodiscard]] TAllocationResult<std::vector<FVulkanTexture>> AllocateTextures(T &&TextureInfo)
	{
		return AllocateTextures(std::span<const FVulkanTextureCreateInfo>{std::addressof(TextureInfo), 1});
	}

	inline VmaAllocator GetVmaAllocator() const
	{
		return Allocator;
	}

  private:
	bool SupportsOptimalHostImageCopy(ETextureFormat Format) const;

	TAllocationResult<FVulkanBuffer> UploadToDeviceLocal(const FVulkanBufferCreateInfo &InBufferCreateInfo);

	[[nodiscard]] TAllocationResult<std::vector<FVulkanTexture>>
	AllocateTexturesUsingHostImageCopy(std::span<const FVulkanTextureCreateInfo> TextureInfos);

	[[nodiscard]] TAllocationResult<std::vector<FVulkanTexture>>
	AllocateTexturesUsingStagingBuffer(std::span<const FVulkanTextureCreateInfo> TextureInfos);

	[[nodiscard]] TAllocationResult<std::pair<VkImage, VmaAllocation>>
	CreateImage(const FVulkanTextureCreateInfo &TextureInfo, bool bHostImageCopy = false);

	VmaAllocator Allocator = VK_NULL_HANDLE;
	FStagingAllocator StagingAllocator;

	vk::PhysicalDevice Gpu = VK_NULL_HANDLE;

	vk::Device Device = VK_NULL_HANDLE;

	// owned by FVulkanDevice
	FVulkanQueue *TransferQueue = nullptr;
	// owned by TransferQueue
	FVulkanCommandBuffer *CommandBuffer = nullptr;

	FVulkanFence *Fence = nullptr;

	bool bHostImageCopySupported = false;
	std::array<bool, (uint8_t)ETextureFormat::Count> OptimalHostImageCopySupport{};
};
