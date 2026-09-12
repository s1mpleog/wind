#pragma once

#include "VulkanBuffer.hpp"
#include "VulkanTexture.hpp"

#include <concepts>
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
