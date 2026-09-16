#include "VulkanAllocator.hpp"

#include "VulkanAllocationError.hpp"
#include "VulkanCheck.hpp"
#include "VulkanCommandBuffer.hpp"
#include "VulkanQueue.hpp"
#include "VulkanSynchronization.hpp"

#include <iterator>
#include <ranges>
#include <spdlog/spdlog.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

bool FVulkanAllocator::SupportsOptimalHostImageCopy(ETextureFormat Format) const
{
	vk::PhysicalDeviceImageFormatInfo2 FormatInfo{};
	FormatInfo.format = ToVk(Format);
	FormatInfo.type = vk::ImageType::e2D;
	FormatInfo.tiling = vk::ImageTiling::eOptimal;
	FormatInfo.usage = vk::ImageUsageFlagBits::eSampled |
	                   (bHostImageCopySupported ? vk::ImageUsageFlagBits::eHostTransfer : vk::ImageUsageFlags{});

	auto Chain =
	    Gpu.getImageFormatProperties2<vk::ImageFormatProperties2, vk::HostImageCopyDevicePerformanceQuery>(FormatInfo);

	vk::HostImageCopyDevicePerformanceQuery &PerfQuery = Chain->get<vk::HostImageCopyDevicePerformanceQuery>();

	if (PerfQuery.optimalDeviceAccess)
	{
		spdlog::info("optimal to use host image copy");
		return true;
	}

	spdlog::info("not optimal to use host image copy");
	return false;
};

FVulkanAllocator::FVulkanAllocator(const vk::Instance Instance, const vk::PhysicalDevice InGpu, vk::Device InDevice,
                                   FVulkanQueue *InTransferQueue, FVulkanFence *InFence, bool bInHostImageCopySupported)
    : Gpu(InGpu), Device(InDevice), TransferQueue(InTransferQueue), Fence(InFence),
      bHostImageCopySupported(bInHostImageCopySupported)
{
	VmaVulkanFunctions Functions{};

	// todo: hard-code version for now
	VmaAllocatorCreateInfo AllocatorInfo{
	    .physicalDevice = Gpu, .device = Device, .instance = Instance, .vulkanApiVersion = VK_API_VERSION_1_3};

	VERIFYVULKANRESULT(vmaImportVulkanFunctionsFromVolk(&AllocatorInfo, &Functions));

	AllocatorInfo.pVulkanFunctions = &Functions;

	VERIFYVULKANRESULT(vmaCreateAllocator(&AllocatorInfo, &Allocator));

	spdlog::info("VMA allocator created");

	CommandBuffer = TransferQueue->AcquireCommandBufferPool()->Create();

	// create signaled fence
	Fence->Create(true);

	// populates which texture format supports optimal host copy
	for (uint8_t Index = 0; Index < OptimalHostImageCopySupport.size(); ++Index)
	{
		OptimalHostImageCopySupport[Index] = SupportsOptimalHostImageCopy(static_cast<ETextureFormat>(Index));
	}
}

// TODO: expected for error handling
TAllocationResult<FVulkanBuffer> FStagingAllocator::Upload(VmaAllocator Allocator, std::span<const std::byte> Data)
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
		return std::unexpected(TranslateAllocationError(Result));
	};

	std::memcpy(AllocationInfo.pMappedData, Data.data(), Data.size());

	return FVulkanBuffer{
	    Allocator,
	    Buffer,
	    Allocation,
	};
}

TAllocationResult<FVulkanBuffer>
FVulkanAllocator::UploadToDeviceLocal(const FVulkanBufferCreateInfo &InBufferCreateInfo)
{
	const bool bIsUniformType = InBufferCreateInfo.Type == EBufferType::Uniform;

	vk::BufferCreateInfo BufferInfo{};
	// don't use TransferDst bit if buffer type is not uniform since it does need to be transfered to gpu
	BufferInfo.usage = ToVk(InBufferCreateInfo.Type) |
	                   (!bIsUniformType ? vk::BufferUsageFlagBits::eTransferDst : vk::BufferUsageFlags{});

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

	if (VkResult Result = vmaCreateBuffer(Allocator, BufferInfo, &AllocationCreateInfo, &Buffer, &Allocation,
	                                      bIsUniformType ? &AllocationInfo : nullptr);
	    Result != VK_SUCCESS)
	{
		return std::unexpected(TranslateAllocationError(Result));
	};

	// memcpy if its uniform buffer and data is not empty
	if (bIsUniformType && !InBufferCreateInfo.Data.empty())
	{
		std::memcpy(AllocationInfo.pMappedData, InBufferCreateInfo.Data.data(), InBufferCreateInfo.Data.size());
	}

	return FVulkanBuffer{Allocator, Buffer, Allocation, InBufferCreateInfo.Type,
	                     bIsUniformType ? AllocationInfo.pMappedData : nullptr};
}

[[nodiscard]] TAllocationResult<std::vector<FVulkanBuffer>>
FVulkanAllocator::AllocateBuffers(std::span<const FVulkanBufferCreateInfo> BufferInfos)
{
	// during first allocation if fence is not signaled then it can hang forever
	// so assuming initial fence is signaled we can wait for it at first it will return instant since
	// initial fence is signaled then reset it to un-signaled and do operation

	// wait for fence to be signaled
	Fence->Wait();
	// reset the fence to un-signaled because we can not submit with signaled fence
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
			TAllocationResult<FVulkanBuffer> StagingBuffer = StagingAllocator.Upload(Allocator, BufferInfo.Data);

			if (!StagingBuffer)
			{
				return std::unexpected(StagingBuffer.error());
			}

			StagingBuffers.push_back(std::move(*StagingBuffer));

			TAllocationResult<FVulkanBuffer> DeviceLocalBuffer = UploadToDeviceLocal(BufferInfo);

			if (!DeviceLocalBuffer)
			{
				return std::unexpected(DeviceLocalBuffer.error());
			}

			OutBuffers.push_back(std::move(*DeviceLocalBuffer));

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
			TAllocationResult<FVulkanBuffer> DeviceLocalBuffer = UploadToDeviceLocal(BufferInfo);

			if (!DeviceLocalBuffer)
			{
				return std::unexpected(DeviceLocalBuffer.error());
			}

			OutBuffers.push_back(std::move(*DeviceLocalBuffer));
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
		return std::unexpected(TranslateAllocationError(SubmitResult.error()));
	}

	spdlog::info("created buffer");

	// wait for gpu to finish before cleaning up staging buffer
	Fence->Wait();

	return OutBuffers;
} // staging will get destroyed here automatically

struct FTextureUploadContext
{
	vk::Device Device = VK_NULL_HANDLE;
	VmaAllocator Allocator = VK_NULL_HANDLE;
	VmaAllocation Allocation = VK_NULL_HANDLE;
	vk::Image Image = VK_NULL_HANDLE;
	vk::ImageView ImageView = VK_NULL_HANDLE;
	vk::Sampler Sampler = VK_NULL_HANDLE;

	FTextureUploadContext() = default;

	FTextureUploadContext(const FTextureUploadContext &) = delete;
	FTextureUploadContext &operator=(const FTextureUploadContext &) = delete;

	FTextureUploadContext(FTextureUploadContext &&Other) noexcept
	    : Device(Other.Device), Allocator(Other.Allocator), Allocation(std::exchange(Other.Allocation, VK_NULL_HANDLE)),
	      Image(std::exchange(Other.Image, VK_NULL_HANDLE)), ImageView(std::exchange(Other.ImageView, VK_NULL_HANDLE)),
	      Sampler(std::exchange(Other.Sampler, VK_NULL_HANDLE)) {};

	FTextureUploadContext &operator=(FTextureUploadContext &&) = delete;

	~FTextureUploadContext()
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
	}
};

[[nodiscard]] TAllocationResult<std::pair<VkImage, VmaAllocation>>
FVulkanAllocator::CreateImage(const FVulkanTextureCreateInfo &TextureInfo, bool bHostImageCopy)
{
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
	ImageInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled |
	                  (bHostImageCopy ? vk::ImageUsageFlagBits::eHostTransferEXT : vk::ImageUsageFlags{});

	VmaAllocationCreateInfo ImageAllocationInfo{};
	ImageAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

	VmaAllocation ImageAllocation{};
	VkImage Image{};

	if (VkResult Result = vmaCreateImage(Allocator, ImageInfo, &ImageAllocationInfo, &Image, &ImageAllocation, nullptr);
	    Result != VK_SUCCESS)
	{
		return std::unexpected(TranslateAllocationError(Result));
	}

	return std::make_pair(Image, ImageAllocation);
}

[[nodiscard]] TAllocationResult<std::vector<FVulkanTexture>>
FVulkanAllocator::AllocateTexturesUsingHostImageCopy(std::span<const FVulkanTextureCreateInfo> TextureInfos)
{
	std::vector<FVulkanTexture> OutTextures;
	OutTextures.reserve(TextureInfos.size());

	for (auto &&[Index, TextureInfo] : std::views::enumerate(TextureInfos))
	{
		if (TextureInfo.Width == 0 || TextureInfo.Height == 0)
		{
			return std::unexpected(EAllocationError::InvalidCreateInfo);
		}

		TAllocationResult<std::pair<VkImage, VmaAllocation>> ImageResult = CreateImage(TextureInfo, true);

		if (!ImageResult)
		{
			return std::unexpected(ImageResult.error());
		}

		auto &[Image, ImageAllocation] = *ImageResult;

		FVulkanTexture Texture{Allocator, Device};
		Texture.Image = Image;
		Texture.Allocation = ImageAllocation;

		// host to image copy
		vk::MemoryToImageCopyEXT Region{};
		Region.pHostPointer = TextureInfo.Pixels.data();
		Region.imageExtent = vk::Extent3D{TextureInfo.Width, TextureInfo.Height, 1};
		Region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		Region.imageSubresource.layerCount = 1;

		vk::CopyMemoryToImageInfoEXT MemoryToImage{};
		MemoryToImage.dstImage = Texture.Image;
		MemoryToImage.dstImageLayout = vk::ImageLayout::eGeneral;
		MemoryToImage.regionCount = 1;
		MemoryToImage.pRegions = &Region;

		// VK_EXT_host_image_copy also introduces a simplified way of doing the required image transition on the
		// host This no longer requires a dedicated command buffer to submit the barrier We also no longer need
		// multiple transitions, and only have to do one for the final layout

		vk::HostImageLayoutTransitionInfoEXT ImageLayoutTransition{};
		ImageLayoutTransition.oldLayout = vk::ImageLayout::eUndefined;
		ImageLayoutTransition.newLayout = vk::ImageLayout::eGeneral;
		ImageLayoutTransition.image = Texture.Image;
		ImageLayoutTransition.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

		// transition layout from undefined -> general and since we are using
		// VK_KHR_unified_image_layouts using general have no performance penalty
		if (auto TransitionResult = Device.transitionImageLayoutEXT(ImageLayoutTransition); !TransitionResult)
		{
			return std::unexpected(TranslateAllocationError(TransitionResult.error()));
		}

		if (auto CopyResult = Device.copyMemoryToImageEXT(MemoryToImage); !CopyResult)
		{
			return std::unexpected(TranslateAllocationError(CopyResult.error()));
		}

		vk::ImageViewCreateInfo ImageViewInfo{};
		ImageViewInfo.format = ToVk(TextureInfo.Format);
		ImageViewInfo.image = Texture.Image;
		ImageViewInfo.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
		ImageViewInfo.viewType = vk::ImageViewType::e2D;

		auto ImageView = Device.createImageView(ImageViewInfo);

		if (!ImageView)
		{
			return std::unexpected(TranslateAllocationError(ImageView.error()));
		}

		Texture.ImageView = *ImageView;

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

		auto Sampler = Device.createSampler(SamplerInfo);

		if (!Sampler)
		{
			return std::unexpected(TranslateAllocationError(Sampler.error()));
		}

		Texture.Sampler = *Sampler;
		Texture.Extent = vk::Extent2D{TextureInfo.Width, TextureInfo.Height};
		Texture.Format = ToVk(TextureInfo.Format);

		OutTextures.push_back(std::move(Texture));
	}

	return OutTextures;
}

[[nodiscard]] TAllocationResult<std::vector<FVulkanTexture>>
FVulkanAllocator::AllocateTexturesUsingStagingBuffer(std::span<const FVulkanTextureCreateInfo> TextureInfos)
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
		if (TextureInfo.Width == 0 || TextureInfo.Height == 0)
		{
			return std::unexpected(EAllocationError::InvalidCreateInfo);
		}

		TAllocationResult<FVulkanBuffer> StagingBuffer = StagingAllocator.Upload(Allocator, TextureInfo.Pixels);

		if (!StagingBuffer)
		{
			return std::unexpected(StagingBuffer.error());
		}

		StagingBuffers.push_back(std::move(*StagingBuffer));

		//===========================Create Image=====================================
		TAllocationResult<std::pair<VkImage, VmaAllocation>> ImageResult = CreateImage(TextureInfo, false);

		if (!ImageResult)
		{
			return std::unexpected(ImageResult.error());
		}

		auto &[Image, ImageAllocation] = *ImageResult;

		FTextureUploadContext Context{};
		Context.Device = Device;
		Context.Allocator = Allocator;
		Context.Allocation = ImageAllocation;
		Context.Image = Image;

		spdlog::info("image is: {}", (void *)Context.Image);

		//=====================Create Image View=============================
		vk::ImageViewCreateInfo ImageViewInfo{};
		ImageViewInfo.format = ToVk(TextureInfo.Format);
		ImageViewInfo.image = Context.Image;
		// todo: don't hard code
		ImageViewInfo.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
		ImageViewInfo.viewType = vk::ImageViewType::e2D;

		auto ImageView = Device.createImageView(ImageViewInfo);

		if (!ImageView)
		{
			return std::unexpected(TranslateAllocationError(ImageView.error()));
		}

		Context.ImageView = *ImageView;

		//===============Transition the image from undefined to general==========
		vk::ImageMemoryBarrier2 TransitionBarrier{};
		TransitionBarrier.image = Context.Image;

		TransitionBarrier.oldLayout = vk::ImageLayout::eUndefined;
		TransitionBarrier.newLayout = vk::ImageLayout::eGeneral;

		TransitionBarrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

		TransitionBarrier.srcStageMask = vk::PipelineStageFlagBits2::eNone;
		TransitionBarrier.srcAccessMask = vk::AccessFlagBits2::eNone;

		TransitionBarrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
		TransitionBarrier.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;

		TransitionBarriers.push_back(std::move(TransitionBarrier));

		UploadContext.push_back(std::move(Context));
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

		CopyInfo.dstImageLayout = vk::ImageLayout::eGeneral;
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

		auto Sampler = Device.createSampler(SamplerInfo);

		if (!Sampler)
		{
			return std::unexpected(TranslateAllocationError(Sampler.error()));
		}

		FVulkanTexture Texture{Allocator,
		                       UploadContext[Index].Allocation,
		                       Device,
		                       UploadContext[Index].Image,
		                       UploadContext[Index].ImageView,
		                       *Sampler,
		                       ToVk(TextureInfo.Format),
		                       vk::Extent2D{TextureInfo.Width, TextureInfo.Height}};

		// todo: i don't like this we have to do this to prevent double free find a better solution
		UploadContext[Index].Image = VK_NULL_HANDLE;
		UploadContext[Index].ImageView = VK_NULL_HANDLE;
		UploadContext[Index].Sampler = VK_NULL_HANDLE;
		UploadContext[Index].Allocation = VK_NULL_HANDLE;

		OutTextures.push_back(std::move(Texture));
	}

	//===========Submit===============
	CommandBuffer->End();

	vk::CommandBufferSubmitInfo CmdBufferSubmitInfo{};
	CmdBufferSubmitInfo.commandBuffer = CommandBuffer->GetHandle();

	vk::SubmitInfo2 SubmitInfo{};
	SubmitInfo.commandBufferInfoCount = 1;
	SubmitInfo.pCommandBufferInfos = &CmdBufferSubmitInfo;

	auto SubmitResult = TransferQueue->GetHandle().submit2(SubmitInfo, Fence->GetHandle());

	if (!SubmitResult)
	{
		return std::unexpected(TranslateAllocationError(SubmitResult.error()));
	}

	// wait for gpu to finish before cleaning up staging buffer
	Fence->Wait();

	return OutTextures;
}

[[nodiscard]] TAllocationResult<std::vector<FVulkanTexture>>
FVulkanAllocator::AllocateTextures(std::span<const FVulkanTextureCreateInfo> TextureInfos)
{
	if (bHostImageCopySupported)
	{
		spdlog::info("allocating texture using staging buffer");
		return AllocateTexturesUsingStagingBuffer(TextureInfos);
	}

	// todo: maybe i don't need these complexity since the offline cooker gives us BC* formats only and they
	// almost optimal host copy i can avoid these dances

	std::vector<FVulkanTextureCreateInfo> HostCopyBatch;
	std::vector<FVulkanTextureCreateInfo> StagingBatch;

	// todo: is this good ? should we reserve here
	HostCopyBatch.reserve(TextureInfos.size());
	StagingBatch.reserve(TextureInfos.size());

	std::ranges::partition_copy(TextureInfos, std::back_inserter(HostCopyBatch), std::back_inserter(StagingBatch),
	                            [&](const FVulkanTextureCreateInfo &Info)
	                            { return OptimalHostImageCopySupport[(uint8_t)Info.Format]; });

	// fast path
	if (StagingBatch.empty())
	{
		spdlog::info("Allocating textures using host image copy");
		return AllocateTexturesUsingHostImageCopy(HostCopyBatch);
	}

	if (HostCopyBatch.empty())
	{
		return AllocateTexturesUsingStagingBuffer(StagingBatch);
	}

	// slow path another vector
	std::vector<FVulkanTexture> OutTextures;
	OutTextures.reserve(TextureInfos.size());

	// FIXME: if ordering matters e.g. TextureInfos[i] -> OutTextures[i] then this is A bug
	TAllocationResult<std::vector<FVulkanTexture>> HostTextures = AllocateTexturesUsingHostImageCopy(HostCopyBatch);

	if (!HostTextures)
	{
		return std::unexpected(HostTextures.error());
	}

	// FVulkanTexture is a RAII handle so i don't have to destroy it manually in error block
	TAllocationResult<std::vector<FVulkanTexture>> StagingTextures = AllocateTexturesUsingStagingBuffer(StagingBatch);

	if (!StagingTextures)
	{
		return std::unexpected(StagingTextures.error());
	}

	// append_range will try to make copy by default but since FVulkanTexture texture is non-copyable
	// it will fail so we have to use as_rvalue so it can invoke move constructor instead
	OutTextures.append_range(*HostTextures | std::views::as_rvalue);
	OutTextures.append_range(*StagingTextures | std::views::as_rvalue);

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
