#pragma once

#include "Config.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Context.hpp"
#include "Vulkan/Graphics/PipelineConfig.hpp"
#include "Vulkan/Memory/ResourceTypes.hpp"
#include "spdlog/spdlog.h"

#include <cstddef>
#include <span>
#include <utility>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

class UGpuAllocator
{
  public:
	UGpuAllocator(const UGpuAllocator &) = delete;
	auto operator=(const UGpuAllocator &) -> UGpuAllocator & = delete;

	UGpuAllocator(UGpuAllocator &&Other) WIND_NOEXCEPT : MAllocator{std::exchange(Other.MAllocator, VK_NULL_HANDLE)},
	                                                     MCommandBuffer{std::move(Other.MCommandBuffer)},
	                                                     MFence{std::move(Other.MFence)}
	{
	}

	auto operator=(UGpuAllocator &&Other) WIND_NOEXCEPT->UGpuAllocator &
	{
		if (this != &Other)
		{
			if (MAllocator != VK_NULL_HANDLE)
				vmaDestroyAllocator(MAllocator);

			MAllocator = std::exchange(Other.MAllocator, VK_NULL_HANDLE);

			MCommandBuffer = std::move(Other.MCommandBuffer);
			MFence = std::move(Other.MFence);
		}

		return *this;
	}

	~UGpuAllocator()
	{
		if (MAllocator != VK_NULL_HANDLE)
		{
			vmaDestroyAllocator(MAllocator);
			spdlog::info("vma allocator destroyed");
		}
	}

	WIND_NODISCARD static auto Create(const FVulkanContext *Context) WIND_NOEXCEPT -> WindResult<UGpuAllocator>;
	WIND_NODISCARD auto UploadStagingBuffer(std::span<const std::byte> Data) WIND_NOEXCEPT
	    -> WindResult<FAllocatedBuffer>;

	// TODO: use my custom types for flags
	WIND_NODISCARD auto CreateBuffers(const FVulkanContext *Context, std::span<const FBufferData> Buffers) WIND_NOEXCEPT
	    -> WindResult<std::vector<FAllocatedBuffer>>;

	WIND_NODISCARD auto CreateBuffer(const FVulkanContext *Context, const FBufferData &Buffer) WIND_NOEXCEPT
	    -> WindResult<FAllocatedBuffer>;

	// TODO: maybe take ownership of data instead of view ?
	WIND_NODISCARD auto CreateTexture(const FVulkanContext *Context,
	                                  std::span<const FTextureData> TextureData) WIND_NOEXCEPT
	    -> WindResult<std::vector<FAllocatedTexture>>;

	WIND_NODISCARD auto CreateVkImage(u32 Width, u32 Height, EFormat Format,
	                                  vk::ImageUsageFlags Usage = vk::ImageUsageFlagBits::eTransferDst |
	                                                              vk::ImageUsageFlagBits::eSampled) WIND_NOEXCEPT
	    -> WindResult<std::pair<VkImage, VmaAllocation>>;

	WIND_NODISCARD auto CreateDepthBuffer(const FVulkanContext *Context, u32 Width, u32 Height) WIND_NOEXCEPT
	    -> WindResult<FAllocatedImage>;

	WIND_NODISCARD auto BeginCommandBuffer() WIND_NOEXCEPT -> WindResult<void>;
	WIND_NODISCARD auto EndCommandBuffer() WIND_NOEXCEPT -> WindResult<void>;
	WIND_NODISCARD auto WaitForFence(const vk::raii::Device &Device) WIND_NOEXCEPT -> WindResult<void>;
	WIND_NODISCARD auto ResetFence(const vk::raii::Device &Device) WIND_NOEXCEPT -> WindResult<void>;
	auto IsFenceSignaled(const vk::raii::Device &Device) WIND_NOEXCEPT -> bool;

	WIND_NODISCARD auto
	CreateDynamicBuffer(u32 Size, vk::BufferUsageFlagBits Usage = vk::BufferUsageFlagBits::eUniformBuffer) WIND_NOEXCEPT
	    -> WindResult<FAllocatedBuffer>;

  private:
	UGpuAllocator(VmaAllocator Allocator, vk::raii::CommandBuffer CommandBuffer, vk::raii::Fence Fence)
	    : MAllocator{Allocator}, MCommandBuffer{std::move(CommandBuffer)}, MFence{std::move(Fence)} {};

	VmaAllocator MAllocator{VK_NULL_HANDLE};
	vk::raii::CommandBuffer MCommandBuffer{nullptr};
	vk::raii::Fence MFence{nullptr};
};
