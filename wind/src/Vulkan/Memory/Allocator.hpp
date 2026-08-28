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

class FUGpuAllocator
{
  public:
	FUGpuAllocator(const FUGpuAllocator &) = delete;
	auto operator=(const FUGpuAllocator &) -> FUGpuAllocator & = delete;

	FUGpuAllocator(FUGpuAllocator &&Other) WIND_NOEXCEPT : Allocator{std::exchange(Other.Allocator, VK_NULL_HANDLE)},
	                                                     CommandBuffer{std::move(Other.CommandBuffer)},
	                                                     Fence{std::move(Other.Fence)}
	{
	}

	auto operator=(FUGpuAllocator &&Other) WIND_NOEXCEPT->FUGpuAllocator &
	{
		if (this != &Other)
		{
			if (Allocator != VK_NULL_HANDLE)
				vmaDestroyAllocator(Allocator);

			Allocator = std::exchange(Other.Allocator, VK_NULL_HANDLE);

			CommandBuffer = std::move(Other.CommandBuffer);
			Fence = std::move(Other.Fence);
		}

		return *this;
	}

	~FUGpuAllocator()
	{
		if (Allocator != VK_NULL_HANDLE)
		{
			vmaDestroyAllocator(Allocator);
			spdlog::info("vma allocator destroyed");
		}
	}

	WIND_NODISCARD static auto Create(const FVulkanContext *Context) WIND_NOEXCEPT -> TWindResult<FUGpuAllocator>;
	WIND_NODISCARD auto UploadStagingBuffer(std::span<const std::byte> Data) WIND_NOEXCEPT
	    -> TWindResult<FAllocatedBuffer>;

	// TODO: use my custom types for flags
	WIND_NODISCARD auto CreateBuffers(const FVulkanContext *Context, std::span<const FBufferData> Buffers) WIND_NOEXCEPT
	    -> TWindResult<std::vector<FAllocatedBuffer>>;

	WIND_NODISCARD auto CreateBuffer(const FVulkanContext *Context, const FBufferData &Buffer) WIND_NOEXCEPT
	    -> TWindResult<FAllocatedBuffer>;

	// TODO: maybe take ownership of data instead of view ?
	WIND_NODISCARD auto CreateTexture(const FVulkanContext *Context,
	                                  std::span<const FTextureData> TextureData) WIND_NOEXCEPT
	    -> TWindResult<std::vector<FAllocatedTexture>>;

	WIND_NODISCARD auto CreateDepthBuffer(const FVulkanContext *Context, TU32 Width, TU32 Height) WIND_NOEXCEPT
	    -> TWindResult<FAllocatedImage>;

	WIND_NODISCARD auto
	CreateDynamicBuffer(TU32 Size,
	                    vk::BufferUsageFlagBits Usage = vk::BufferUsageFlagBits::eUniformBuffer) WIND_NOEXCEPT
	    -> TWindResult<FAllocatedBuffer>;

  private:
	WIND_NODISCARD auto BeginCommandBuffer() WIND_NOEXCEPT -> TWindResult<void>;
	WIND_NODISCARD auto EndCommandBuffer() WIND_NOEXCEPT -> TWindResult<void>;
	WIND_NODISCARD auto WaitForFence(const vk::raii::Device &Device) WIND_NOEXCEPT -> TWindResult<void>;
	WIND_NODISCARD auto ResetFence(const vk::raii::Device &Device) WIND_NOEXCEPT -> TWindResult<void>;
	auto IsFenceSignaled(const vk::raii::Device &Device) WIND_NOEXCEPT -> bool;

	WIND_NODISCARD auto CreateVkImage(TU32 Width, TU32 Height, EFormat Format,
	                                  vk::ImageUsageFlags Usage = vk::ImageUsageFlagBits::eTransferDst |
	                                                              vk::ImageUsageFlagBits::eSampled) WIND_NOEXCEPT
	    -> TWindResult<std::pair<VkImage, VmaAllocation>>;

	FUGpuAllocator(VmaAllocator Allocator, vk::raii::CommandBuffer CommandBuffer, vk::raii::Fence Fence)
	    : Allocator{Allocator}, CommandBuffer{std::move(CommandBuffer)}, Fence{std::move(Fence)} {};

	VmaAllocator Allocator{VK_NULL_HANDLE};
	vk::raii::CommandBuffer CommandBuffer{nullptr};
	vk::raii::Fence Fence{nullptr};
};
