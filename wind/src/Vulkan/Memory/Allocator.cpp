#include "Vulkan/Memory/Allocator.hpp"

#include "Config.hpp"
#include "Error.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Synchroization.hpp"
#include "Vulkan/Graphics/PipelineConfig.hpp"
#include "Vulkan/Memory/ResourceTypes.hpp"
#include "Vulkan/Types.hpp"
#include "spdlog/spdlog.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ranges>
#include <span>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_to_string.hpp>

WIND_NODISCARD auto UGpuAllocator::Create(const FVulkanContext *Context) WIND_NOEXCEPT -> TWindResult<UGpuAllocator>
{
	VmaAllocatorCreateInfo AllocatorInfo{};

	AllocatorInfo.physicalDevice = *Context->GpuDevice.PhysicalDevice;
	AllocatorInfo.device = *Context->GpuDevice.Device;
	AllocatorInfo.preferredLargeHeapBlockSize = 0;
	AllocatorInfo.instance = *Context->Instance;
	AllocatorInfo.vulkanApiVersion = Context->GpuDevice.PhysicalDeviceProps.apiVersion;

	VmaAllocator TempAllocator{};

	if (auto Result = vmaCreateAllocator(&AllocatorInfo, &TempAllocator); Result != VK_SUCCESS)
		WIND_ERR(WindError::vulkan(ErrorCode::InternalError, static_cast<vk::Result>(Result)));

#ifdef WIND_LOG_ENABLE
	spdlog::info("VMA allocator created successfully");
#endif

	vk::CommandBufferAllocateInfo CommandBufferAllocInfo{};
	CommandBufferAllocInfo.commandBufferCount = 1;
	CommandBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	// use transfer pool if available otherwise use fallback to graphics pool
	CommandBufferAllocInfo.commandPool = Context->GpuDevice.HasTransferQueue() ? Context->GpuDevice.TransferPool.value()
	                                                                           : Context->GpuDevice.GraphicsPool;

	auto CommandBuffers = WIND_TRY(Context->GpuDevice.Device.allocateCommandBuffers(CommandBufferAllocInfo),
	                               ErrorCode::FailedToAllocateCommandBuffer);

	auto Fence =
	    WIND_TRY(Context->GpuDevice.Device.createFence(vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled}),
	             ErrorCode::FailedToCreateFence);

	return UGpuAllocator{TempAllocator, std::move(CommandBuffers[0]), std::move(Fence)};
}

WIND_NODISCARD auto UGpuAllocator::UploadStagingBuffer(std::span<const std::byte> Data) WIND_NOEXCEPT
    -> TWindResult<FAllocatedBuffer>
{
	const vk::DeviceSize Size = Data.size();

	vk::BufferCreateInfo BufferInfo{};
	BufferInfo.size = Size;
	BufferInfo.sharingMode = vk::SharingMode::eExclusive;
	BufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;

	// first create a staging buffer which CPU can access it and write to it
	VmaAllocationCreateInfo AllocationCreateInfo{};
	AllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	AllocationCreateInfo.flags =
	    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VmaAllocationInfo AllocInfo{};
	VkBuffer Buffer{};
	VmaAllocation Allocation{};

	// creates a new vkBuffer, allocates and binds memory for it
	if (auto Result = vmaCreateBuffer(Allocator, BufferInfo, &AllocationCreateInfo, &Buffer, &Allocation, &AllocInfo);
	    Result != VK_SUCCESS)
		WIND_ERR(WindError::vulkan(ErrorCode::FailedToCreateBuffer, static_cast<vk::Result>(Result)));

	// copy our data to staging buffer
	std::memcpy(AllocInfo.pMappedData, Data.data(), Size);

	return FAllocatedBuffer{
	    Buffer,
	    Allocation,
	    Allocator,
	};
}

WIND_NODISCARD auto UGpuAllocator::BeginCommandBuffer() WIND_NOEXCEPT -> TWindResult<void>
{
	WIND_ASSERT(CommandBuffer != nullptr && "Command buffer is null");

	vk::CommandBufferBeginInfo BeginInfo{};
	BeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	WIND_TRY(CommandBuffer.begin(BeginInfo), ErrorCode::FailedToBeginCommandBuffer);

	return {};
}

WIND_NODISCARD auto UGpuAllocator::EndCommandBuffer() WIND_NOEXCEPT -> TWindResult<void>
{
	WIND_ASSERT(CommandBuffer != nullptr && "Command buffer is null");

	WIND_TRY(CommandBuffer.end());
	return {};
}

WIND_NODISCARD auto UGpuAllocator::WaitForFence(const vk::raii::Device &Device) WIND_NOEXCEPT -> TWindResult<void>
{
	WIND_ASSERT(Fence != nullptr && "fence is null");
	auto Result = Device.waitForFences(*Fence, vk::True, UINT64_MAX);

	if (Result != vk::Result::eSuccess)
		WIND_ERR(WindError::vulkan(ErrorCode::FailedToWaitForFence, Result));

	return {};
}

WIND_NODISCARD auto UGpuAllocator::ResetFence(const vk::raii::Device &Device) WIND_NOEXCEPT -> TWindResult<void>
{
	WIND_TRY(Device.resetFences(*Fence));
	return {};
}

WIND_NODISCARD auto UGpuAllocator::CreateBuffers(const FVulkanContext *Context,
                                                 std::span<const FBufferData> Buffers) WIND_NOEXCEPT
    -> TWindResult<std::vector<FAllocatedBuffer>>
{
	WIND_ASSERT(Allocator != VK_NULL_HANDLE && "VMA allocator is null");
	WIND_ASSERT(Context != VK_NULL_HANDLE && "context is null");

	std::vector<FAllocatedBuffer> AllocatedBuffers;
	AllocatedBuffers.reserve(Buffers.size());

	std::vector<FAllocatedBuffer> StagingBuffers;
	StagingBuffers.reserve(Buffers.size());

	WIND_TRY(BeginCommandBuffer());

	WIND_TRY(WaitForFence(Context->GpuDevice.Device));
	WIND_TRY(ResetFence(Context->GpuDevice.Device));

	for (auto &&[index, buffer] : std::views::enumerate(Buffers))
	{
		const vk::DeviceSize Size = buffer.Data.size();
		StagingBuffers.emplace_back(WIND_TRY(UploadStagingBuffer(buffer.Data)));

		spdlog::info("staging buffer created: {}", reinterpret_cast<void *>(StagingBuffers[index].Buffer));

		vk::BufferCreateInfo BufferCreateInfo{};
		BufferCreateInfo.size = Size;

		// transferDst because we are copying from staging buffer and eVertexBuffer because we want to use this
		// as what ever flags is
		vk::BufferUsageFlags Usage = buffer.Usage | vk::BufferUsageFlagBits::eTransferDst;
		BufferCreateInfo.usage = Usage;
		BufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

		VmaAllocationCreateInfo AllocationCreateInfo{};
		AllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

		VmaAllocation ImageAllocation{};
		VkBuffer DeviceBuffer{};

		if (auto Result = vmaCreateBuffer(Allocator, BufferCreateInfo, &AllocationCreateInfo, &DeviceBuffer,
		                                  &ImageAllocation, nullptr);
		    Result != VK_SUCCESS)
		{
			WIND_ERR(WindError::vulkan(ErrorCode::FailedToCreateBuffer, static_cast<vk::Result>(Result)));
		}

		spdlog::info("buffer created: {}", reinterpret_cast<void *>(DeviceBuffer));

		vk::BufferCopy CopyRegion{};
		CopyRegion.size = Size;
		CopyRegion.dstOffset = 0;
		CopyRegion.srcOffset = 0;

		// copy the buffer
		CommandBuffer.copyBuffer(StagingBuffers[index].Buffer, DeviceBuffer, CopyRegion);

		AllocatedBuffers.emplace_back(DeviceBuffer, ImageAllocation, Allocator);
	}

	WIND_TRY(EndCommandBuffer());

	vk::SubmitInfo SubmitInfo{};
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &*CommandBuffer;

	if (Context->GpuDevice.HasTransferQueue())
	{
		WIND_TRY(Context->GpuDevice.TransferQueue->submit(SubmitInfo, *Fence), ErrorCode::FailedToSubmitQueue);
	}
	else
	{
		WIND_TRY(Context->GpuDevice.GraphicsQueue.submit(SubmitInfo, *Fence), ErrorCode::FailedToSubmitQueue);
	}

	WIND_TRY(WaitForFence(Context->GpuDevice.Device));

	return AllocatedBuffers;
}

WIND_NODISCARD auto UGpuAllocator::CreateBuffer(const FVulkanContext *Context, const FBufferData &Buffer) WIND_NOEXCEPT
    -> TWindResult<FAllocatedBuffer>
{
	auto Result = WIND_TRY(CreateBuffers(Context, std::span{&Buffer, 1}));
	WIND_ENSURE_NOT_EMPTY(Result, WindError::vulkan());

	return std::move(Result.front());
}

WIND_NODISCARD auto UGpuAllocator::CreateVkImage(TU32 Width, TU32 Height, EFormat Format,
                                                 vk::ImageUsageFlags Usage) WIND_NOEXCEPT
    -> TWindResult<std::pair<VkImage, VmaAllocation>>
{
	vk::ImageCreateInfo ImageCreateInfo{};
	ImageCreateInfo.imageType = vk::ImageType::e2D;
	ImageCreateInfo.extent = {.width = Width, .height = Height, .depth = 1};
	ImageCreateInfo.format = ToVk(Format);
	ImageCreateInfo.tiling = vk::ImageTiling::eOptimal;
	ImageCreateInfo.samples = vk::SampleCountFlagBits::e1;
	ImageCreateInfo.usage = Usage;
	ImageCreateInfo.mipLevels = 1;
	ImageCreateInfo.arrayLayers = 1;
	ImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

	spdlog::info("creating image: {}x{}, format={}, tiling={}, usage={:#x}, "
	             "mipLevels={}, arrayLayers={}, samples={}",
	             Width, Height, vk::to_string(ImageCreateInfo.format), vk::to_string(ImageCreateInfo.tiling),
	             static_cast<VkImageUsageFlags>(ImageCreateInfo.usage), ImageCreateInfo.mipLevels,
	             ImageCreateInfo.arrayLayers, vk::to_string(ImageCreateInfo.samples));

	VkImageCreateInfo VkImageCreateInfo = ImageCreateInfo;

	VmaAllocationCreateInfo ImageAllocationInfo{};
	ImageAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

	VmaAllocation ImageAllocation{};
	VkImage Image{};

	if (auto Result =
	        vmaCreateImage(Allocator, &VkImageCreateInfo, &ImageAllocationInfo, &Image, &ImageAllocation, nullptr);
	    Result != VK_SUCCESS)
		WIND_ERR(WindError::vulkan(ErrorCode::FailedToCreateImage, static_cast<vk::Result>(Result)));

	return std::pair{Image, ImageAllocation};
}

WIND_NODISCARD auto UGpuAllocator::CreateTexture(const FVulkanContext *Context,
                                                 std::span<const FTextureData> TextureData) WIND_NOEXCEPT
    -> TWindResult<std::vector<FAllocatedTexture>>
{
	WIND_ASSERT(Allocator != VK_NULL_HANDLE && "VMA allocator is null");
	WIND_ASSERT(Context != VK_NULL_HANDLE && "context is null");

	std::vector<FAllocatedBuffer> StagingBuffers;
	StagingBuffers.reserve(TextureData.size());

	std::vector<FAllocatedTexture> Textures;
	Textures.reserve(TextureData.size());

	WIND_TRY(BeginCommandBuffer());

	// return iff STATE == SIGNALED
	WIND_TRY(WaitForFence(Context->GpuDevice.Device));
	// reset state to UN-SIGNALED
	WIND_TRY(ResetFence(Context->GpuDevice.Device));

	for (auto &&[index, data] : std::views::enumerate(TextureData))
	{
		// we can do that since we have to create staging buffer for each data
		StagingBuffers.emplace_back(WIND_TRY(UploadStagingBuffer(data.Pixels)));

		auto [image, image_allocation] =
		    WIND_TRY(CreateVkImage(data.Dimensions.width, data.Dimensions.height, data.Format));

		spdlog::info("image created successfully: {}", reinterpret_cast<void *>(image));

		vk::ImageViewCreateInfo ImageViewCreateInfo{};
		ImageViewCreateInfo.image = image;
		ImageViewCreateInfo.format = ToVk(data.Format);
		ImageViewCreateInfo.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
		ImageViewCreateInfo.viewType = vk::ImageViewType::e2D;

		// create image view
		auto ImageView = WIND_TRY(Context->GpuDevice.Device.createImageView(ImageViewCreateInfo));

		vk::BufferImageCopy Region{};
		Region.imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1};
		Region.imageExtent = {
		    .width = data.Dimensions.width, .height = data.Dimensions.height, .depth = data.Dimensions.depth};

		// transition from undefined to TransferDst
		TransitionImage(CommandBuffer, image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

		CommandBuffer.copyBufferToImage(StagingBuffers[index].Buffer, image, vk::ImageLayout::eTransferDstOptimal,
		                                Region);

		// transition from Transfer dst to shader read
		TransitionImage(CommandBuffer, image, vk::ImageLayout::eTransferDstOptimal,
		                vk::ImageLayout::eShaderReadOnlyOptimal);

		vk::SamplerCreateInfo SamplerCreateInfo{};
		SamplerCreateInfo.anisotropyEnable = vk::False;
		SamplerCreateInfo.magFilter = vk::Filter::eLinear;
		SamplerCreateInfo.minFilter = vk::Filter::eLinear;
		SamplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
		SamplerCreateInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
		SamplerCreateInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
		SamplerCreateInfo.addressModeW = vk::SamplerAddressMode::eRepeat;

		auto Sampler = WIND_TRY(Context->GpuDevice.Device.createSampler(SamplerCreateInfo));

		Textures.emplace_back(FAllocatedImage{image, std::move(ImageView), Allocator, image_allocation,
		                                      ToVk(data.Format),
		                                      vk::Extent2D{
		                                          data.Dimensions.width,
		                                          data.Dimensions.height,
		                                      }},
		                      std::move(Sampler));
	}

	WIND_TRY(EndCommandBuffer());

	vk::SubmitInfo SubmitInfo{};
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &*CommandBuffer;

	if (Context->GpuDevice.HasTransferQueue())
	{
		WIND_TRY(Context->GpuDevice.TransferQueue->submit(SubmitInfo, *Fence), ErrorCode::FailedToSubmitQueue);
	}
	else
	{
		WIND_TRY(Context->GpuDevice.GraphicsQueue.submit(SubmitInfo, *Fence), ErrorCode::FailedToSubmitQueue);
	}

	WIND_TRY(WaitForFence(Context->GpuDevice.Device));

	return Textures;
}

WIND_NODISCARD auto UGpuAllocator::CreateDepthBuffer(const FVulkanContext *Context, TU32 Width,
                                                     TU32 Height) WIND_NOEXCEPT -> TWindResult<FAllocatedImage>
{
	auto [image, image_allocation] =
	    WIND_TRY(CreateVkImage(Width, Height, EFormat::D32Float, vk::ImageUsageFlagBits::eDepthStencilAttachment));

	vk::ImageViewCreateInfo ImageViewCreateInfo{};
	ImageViewCreateInfo.image = image;
	ImageViewCreateInfo.format = ToVk(EFormat::D32Float);
	ImageViewCreateInfo.subresourceRange = {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1};
	ImageViewCreateInfo.viewType = vk::ImageViewType::e2D;

	// create image view
	auto ImageView = WIND_TRY(Context->GpuDevice.Device.createImageView(ImageViewCreateInfo));

	// ================Upload the data to GPU===============

	// set command buffer to recording state
	WIND_TRY(BeginCommandBuffer());

	// transition from undefined dst to depth optimal
	TransitionImage(CommandBuffer, image, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal);

	// end the command buffer no more recording :(
	WIND_TRY(EndCommandBuffer());

	vk::SubmitInfo SubmitInfo{};
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &*CommandBuffer;

	WIND_TRY(WaitForFence(Context->GpuDevice.Device));
	WIND_TRY(ResetFence(Context->GpuDevice.Device));

	if (Context->GpuDevice.HasTransferQueue())
	{
		WIND_TRY(Context->GpuDevice.TransferQueue->submit(SubmitInfo, *Fence), ErrorCode::FailedToSubmitQueue);
	}
	else
	{
		WIND_TRY(Context->GpuDevice.GraphicsQueue.submit(SubmitInfo, *Fence), ErrorCode::FailedToSubmitQueue);
	}

	WIND_TRY(WaitForFence(Context->GpuDevice.Device));

	return FAllocatedImage{
	    image, std::move(ImageView), Allocator, image_allocation, ToVk(EFormat::D32Float), vk::Extent2D{Width, Height}};
}

WIND_NODISCARD auto UGpuAllocator::CreateDynamicBuffer(TU32 Size, vk::BufferUsageFlagBits Usage) WIND_NOEXCEPT
    -> TWindResult<FAllocatedBuffer>
{
	vk::BufferCreateInfo BufferInfo{};
	BufferInfo.usage = Usage;
	BufferInfo.sharingMode = vk::SharingMode::eExclusive;
	BufferInfo.size = vk::DeviceSize{Size};

	VmaAllocationCreateInfo AllocationCreateInfo{};
	AllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
	AllocationCreateInfo.flags =
	    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
	AllocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	VmaAllocation Allocation{};
	VkBuffer DeviceBuffer{};
	VmaAllocationInfo AllocationInfo{};

	if (auto Result =
	        vmaCreateBuffer(Allocator, BufferInfo, &AllocationCreateInfo, &DeviceBuffer, &Allocation, &AllocationInfo);
	    Result != VK_SUCCESS)
	{
		WIND_ERR(WindError::vulkan(ErrorCode::FailedToCreateBuffer, static_cast<vk::Result>(Result)));
	}

	return FAllocatedBuffer{DeviceBuffer, Allocation, Allocator, AllocationInfo.pMappedData};
}
