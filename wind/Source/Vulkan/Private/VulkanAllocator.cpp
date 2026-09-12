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
