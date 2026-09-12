#include "VulkanAllocator.hpp"

#include "VulkanCheck.hpp"
#include "VulkanCommandBuffer.hpp"
#include "VulkanQueue.hpp"
#include "VulkanSynchronization.hpp"

#include <ranges>
#include <spdlog/spdlog.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

FVulkanAllocator::FVulkanAllocator(const vk::Instance Instance, const vk::PhysicalDevice Gpu, vk::Device InDevice,
                                   FVulkanQueue *InTransferQueue, FVulkanFence *InFence)
    : Device(InDevice), TransferQueue(InTransferQueue), Fence(InFence)
{
	VmaVulkanFunctions Functions{};

	// todo: hardcode version for now
	VmaAllocatorCreateInfo AllocatorInfo{
	    .physicalDevice = Gpu, .device = Device, .instance = Instance, .vulkanApiVersion = VK_API_VERSION_1_3};

	VERIFYVULKANRESULT(vmaImportVulkanFunctionsFromVolk(&AllocatorInfo, &Functions));

	AllocatorInfo.pVulkanFunctions = &Functions;

	VERIFYVULKANRESULT(vmaCreateAllocator(&AllocatorInfo, &Allocator));

	spdlog::info("vma allocator created");

	CommandBuffer = TransferQueue->AcquireCommandBufferPool()->Create();

	// create signaled fence
	Fence->Create(true);
}

// TODO: expected for error handling
FVulkanBuffer FStagingAllocator::Upload(VmaAllocator Allocator, std::span<const std::byte> Data)
{
	vk::BufferCreateInfo BufferInfo{};
	BufferInfo.size = vk::DeviceSize{Data.size()};
	BufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	BufferInfo.sharingMode = vk::SharingMode::eExclusive;

	VmaAllocationCreateInfo AllocationCreateInfo{};
	AllocationCreateInfo.flags =
	    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
	AllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

	VmaAllocationInfo AllocationInfo{};
	VkBuffer Buffer{};
	VmaAllocation Allocation{};

	if (VkResult Result =
	        vmaCreateBuffer(Allocator, BufferInfo, &AllocationCreateInfo, &Buffer, &Allocation, &AllocationInfo);
	    Result != VK_SUCCESS)
	{
		spdlog::info("staging buffer allocation failed");
		return {};
	}

	std::memcpy(AllocationInfo.pMappedData, Data.data(), Data.size());

	return FVulkanBuffer{
	    Allocator,
	    Buffer,
	    Allocation,
	};
}

FVulkanBuffer FVulkanAllocator::UploadToDeviceLocal(const FVulkanBufferCreateInfo &InBufferCreateInfo)
{
	const bool bIsUniformType = InBufferCreateInfo.Type == EBufferType::Uniform;

	vk::BufferCreateInfo BufferInfo{};
	// don't use TransferDst bit if buffer type is not uniform since it does need to be transfered to gpu
	BufferInfo.usage = ToVk(InBufferCreateInfo.Type) |=
	    !bIsUniformType ? vk::BufferUsageFlagBits::eTransferDst : vk::BufferUsageFlags{};

	BufferInfo.size = vk::DeviceSize{InBufferCreateInfo.Data.size()};
	BufferInfo.sharingMode = vk::SharingMode::eExclusive;

	VmaAllocationCreateInfo AllocationCreateInfo{};
	AllocationCreateInfo.usage =
	    bIsUniformType ? VMA_MEMORY_USAGE_AUTO_PREFER_HOST : VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	AllocationCreateInfo.flags =
	    bIsUniformType ? VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT : 0;

	VmaAllocationInfo AllocationInfo{};
	VkBuffer Buffer{};
	VmaAllocation Allocation{};

	// todo: handle error with expected
	if (VkResult Result = vmaCreateBuffer(Allocator, BufferInfo, &AllocationCreateInfo, &Buffer, &Allocation,
	                                      bIsUniformType ? &AllocationInfo : nullptr);
	    Result != VK_SUCCESS)
	{
		return {};
	}

	// memcpy if its uniform buffer and data is not empty
	if (bIsUniformType && !InBufferCreateInfo.Data.empty())
	{
		std::memcpy(AllocationInfo.pMappedData, InBufferCreateInfo.Data.data(), InBufferCreateInfo.Data.size());
	}

	return FVulkanBuffer{Allocator, Buffer, Allocation, InBufferCreateInfo.Type,
	                     bIsUniformType ? AllocationInfo.pMappedData : nullptr};
}

[[nodiscard]] std::vector<FVulkanBuffer>
FVulkanAllocator::AllocateBuffers(std::span<const FVulkanBufferCreateInfo> BufferInfos)
{
	// during first allocation if fence is not signaled then it can hang forever
	// so assuming inital fence is signaled we can wait for it at first it will return instant since
	// initial fence is signaled then reset it to unsignaled and do operation

	// wait for fence to be signaled
	Fence->Wait();
	// reset the fence to unsignaled because we can not sumbit with signaled fence
	Fence->Reset();

	vk::CommandBufferBeginInfo BeginInfo{};
	CommandBuffer->Begin(BeginInfo);

	std::vector<FVulkanBuffer> OutBuffers;
	OutBuffers.reserve(BufferInfos.size());

	std::vector<FVulkanBuffer> StagingBuffers;
	StagingBuffers.reserve(BufferInfos.size());

	for (auto &&[Index, BufferInfo] : std::views::enumerate(BufferInfos))
	{
		const bool bNeedsStaging = !BufferInfo.Data.empty() && BufferInfo.Type != EBufferType::Uniform;

		if (bNeedsStaging)
		{
			// upload to staging buffer then copy to device local

			// staging buffer host visible + memcpy
			StagingBuffers.emplace_back(StagingAllocator.Upload(Allocator, BufferInfo.Data));

			// device local buffer !host visible
			OutBuffers.emplace_back(UploadToDeviceLocal(BufferInfo));

			vk::BufferCopy2 CopyRegion{};
			CopyRegion.size = vk::DeviceSize{BufferInfo.Data.size()};
			CopyRegion.dstOffset = 0;
			CopyRegion.srcOffset = 0;

			vk::CopyBufferInfo2 CopyInfo{};
			CopyInfo.srcBuffer = StagingBuffers[Index].Buffer;
			CopyInfo.dstBuffer = OutBuffers[Index].Buffer;
			CopyInfo.regionCount = 1;
			CopyInfo.pRegions = &CopyRegion;

			CommandBuffer->GetHandle().copyBuffer2(CopyInfo);
		}
		else
		{
			// so here we don't have data and buffer type is not uniform
			// just upload to device local that's it
			OutBuffers.emplace_back(UploadToDeviceLocal(BufferInfo));
		}
	}

	CommandBuffer->End();

	vk::CommandBufferSubmitInfo CmdBufferSubmitInfo{};
	CmdBufferSubmitInfo.commandBuffer = CommandBuffer->GetHandle();

	vk::SubmitInfo2 SubmitInfo{};
	SubmitInfo.commandBufferInfoCount = 1;
	SubmitInfo.pCommandBufferInfos = &CmdBufferSubmitInfo;

	auto SubmitResult = TransferQueue->GetHandle().submit2(SubmitInfo, Fence->GetHandle());

	/*
	*                  ONE VkBuffer
	    ┌──────────────────────────────────────────────────────────┐
	    │       buf1       │   buf2   │      buf3      │ buf4 │...│
	    └──────────────────────────────────────────────────────────┘
	    0                  ^          ^                ^
	                       offsets

	 instead of having N staging buffer and N device local buffer
	*/

	// todo: make sure to use expected
	if (!SubmitResult)
	{
		// error
		spdlog::info("submit failed: {}", vk::to_string(SubmitResult.error()));
		return {};
	}

	spdlog::info("created buffer");

	// wait for gpu to finish before cleaning up staging buffer
	Fence->Wait();

	return OutBuffers;
} // staging will get destroyed here automatically

struct FTextureUploadContext
{
	VmaAllocation Allocation = VK_NULL_HANDLE;
	vk::Image Image = VK_NULL_HANDLE;
	vk::ImageView ImageView = VK_NULL_HANDLE;
};

// before using the Texture make sure to transition the Image to the prefered format
// initially the format will be eTransferDstOptimal
[[nodiscard]] std::vector<FVulkanTexture>
FVulkanAllocator::AllocateTextures(std::span<const FVulkanTextureCreateInfo> TextureInfos)
{
	Fence->Wait();

	Fence->Reset();

	vk::CommandBufferBeginInfo CmdBufferBeginInfo{};
	CommandBuffer->Begin(CmdBufferBeginInfo);

	std::vector<FVulkanBuffer> StagingBuffers;
	StagingBuffers.reserve(TextureInfos.size());

	std::vector<FVulkanTexture> OutTextures;
	OutTextures.reserve(TextureInfos.size());

	std::vector<FTextureUploadContext> UploadContext{};
	UploadContext.reserve(TextureInfos.size());

	std::vector<vk::ImageMemoryBarrier2> TransitionBarriers;
	TransitionBarriers.reserve(TextureInfos.size());

	for (auto &&[Index, TextureInfo] : std::views::enumerate(TextureInfos))
	{
		StagingBuffers.emplace_back(StagingAllocator.Upload(Allocator, TextureInfo.Pixels));

		//===========================Create Image======================================
		vk::ImageCreateInfo ImageInfo{};
		ImageInfo.extent = vk::Extent3D{TextureInfo.Width, TextureInfo.Height, 1};
		ImageInfo.format = ToVk(TextureInfo.Format);
		ImageInfo.imageType = vk::ImageType::e2D;
		ImageInfo.initialLayout = vk::ImageLayout::eUndefined;
		ImageInfo.sharingMode = vk::SharingMode::eExclusive;
		ImageInfo.mipLevels = 1;
		ImageInfo.arrayLayers = 1;
		ImageInfo.samples = vk::SampleCountFlagBits::e1;
		ImageInfo.tiling = vk::ImageTiling::eOptimal;
		// todo: accept usage in Info
		ImageInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;

		VmaAllocationCreateInfo ImageAllocationInfo{};
		ImageAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

		VmaAllocation ImageAllocation{};
		VkImage Image{};

		// todo: if image creation failed then skip the current loop
		// in result provide some info which texture create which failed etc
		// and use expected
		if (vmaCreateImage(Allocator, ImageInfo, &ImageAllocationInfo, &Image, &ImageAllocation, nullptr) != VK_SUCCESS)
		{
			spdlog::info("failed to create image");
			return {};
		}

		//=====================Create Image View=============================
		vk::ImageViewCreateInfo ImageViewInfo{};
		ImageViewInfo.format = ToVk(TextureInfo.Format);
		ImageViewInfo.image = Image;
		// todo: don't hardcode
		ImageViewInfo.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
		ImageViewInfo.viewType = vk::ImageViewType::e2D;

		auto ImageView = Device.createImageView(ImageViewInfo);

		if (!ImageView)
		{
			spdlog::info("failed to create image view: {}", vk::to_string(ImageView.error()));
			return {};
		}

		//===============Transition the image from undefined to transferDst optimal==========
		vk::ImageMemoryBarrier2 TransitionBarrier{};

		// ====initial layout is undefined so we don't care about producer side============
		TransitionBarrier.image = Image;

		TransitionBarrier.oldLayout = vk::ImageLayout::eUndefined;
		// pick optimal layout for transfer
		TransitionBarrier.newLayout = vk::ImageLayout::eTransferDstOptimal;

		// todo: later stop hardcoding this i have to support depth image also
		TransitionBarrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

		// Sync scope A happens-before Sync scope B
		TransitionBarrier.srcStageMask = vk::PipelineStageFlagBits2::eNone;
		// producer memory op. this will be visible to consumer (here its none so ignore)
		TransitionBarrier.srcAccessMask = vk::AccessFlagBits2::eNone;

		// consumer side the point is transfer and the operation will be write
		// since we are copying from staging buffer -> image

		// Sync scope B happens after Sync Scope A
		TransitionBarrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
		// consumer memory operation
		TransitionBarrier.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;

		TransitionBarriers.push_back(std::move(TransitionBarrier));

		UploadContext.emplace_back(ImageAllocation, Image, ImageView.value());
	}

	vk::DependencyInfo DepInfo{};
	DepInfo.imageMemoryBarrierCount = static_cast<uint32_t>(TransitionBarriers.size());
	DepInfo.pImageMemoryBarriers = TransitionBarriers.data();

	CommandBuffer->GetHandle().pipelineBarrier2(DepInfo);

	for (auto &&[Index, TextureInfo] : std::views::enumerate(TextureInfos))
	{
		vk::BufferImageCopy2 CopyRegion{};
		CopyRegion.imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1};
		CopyRegion.imageExtent = vk::Extent3D{TextureInfo.Width, TextureInfo.Height, 1};

		vk::CopyBufferToImageInfo2 CopyInfo{};

		// copy from staging buffer to image
		CopyInfo.srcBuffer = StagingBuffers[Index].Buffer;
		CopyInfo.dstImage = UploadContext[Index].Image;

		CopyInfo.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;
		CopyInfo.regionCount = 1;
		CopyInfo.pRegions = &CopyRegion;

		CommandBuffer->GetHandle().copyBufferToImage2(CopyInfo);
	}

	for (auto &&[Index, TextureInfo] : std::views::enumerate(TextureInfos))
	{
		// todo: later do sampler optimization create one sampler instead of N
		vk::SamplerCreateInfo SamplerInfo{};

		SamplerInfo.magFilter = vk::Filter::eLinear;
		SamplerInfo.minFilter = vk::Filter::eLinear;
		SamplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;

		SamplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
		SamplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
		SamplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;

		SamplerInfo.anisotropyEnable = vk::True;
		SamplerInfo.maxAnisotropy = 16.0F;

		SamplerInfo.minLod = 0.0F;
		SamplerInfo.maxLod = vk::LodClampNone;
		SamplerInfo.mipLodBias = 0.0F;

		SamplerInfo.compareEnable = vk::False;
		SamplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
		SamplerInfo.unnormalizedCoordinates = vk::False;

		auto SamplerResult = Device.createSampler(SamplerInfo);

		if (!SamplerResult)
		{
			spdlog::info("failed to create sampler: {}", vk::to_string(SamplerResult.error()));
			return {};
		}

		OutTextures.emplace_back(Allocator, UploadContext[Index].Allocation, Device, UploadContext[Index].Image,
		                         UploadContext[Index].ImageView, SamplerResult.value(), ToVk(TextureInfo.Format),
		                         vk::Extent2D{TextureInfo.Width, TextureInfo.Height});
	}

	//===========Submit===============
	CommandBuffer->End();

	vk::CommandBufferSubmitInfo CmdBufferSubmitInfo{};
	CmdBufferSubmitInfo.commandBuffer = CommandBuffer->GetHandle();

	vk::SubmitInfo2 SubmitInfo{};
	SubmitInfo.commandBufferInfoCount = 1;
	SubmitInfo.pCommandBufferInfos = &CmdBufferSubmitInfo;

	auto SubmitResult = TransferQueue->GetHandle().submit2(SubmitInfo, Fence->GetHandle());

	// todo: make sure to use expected
	if (!SubmitResult)
	{
		// error
		spdlog::info("submit failed: {}", vk::to_string(SubmitResult.error()));
		return {};
	}

	spdlog::info("created texture");

	// wait for gpu to finish before cleaning up staging buffer
	Fence->Wait();

	return OutTextures;
}

FVulkanAllocator::~FVulkanAllocator()
{
	if (Allocator != VK_NULL_HANDLE)
	{
		vmaDestroyAllocator(Allocator);
		Allocator = VK_NULL_HANDLE;
	}

	if (Fence != nullptr)
	{
		Fence->Destroy();
		delete Fence;
		Fence = nullptr;
	}
}
