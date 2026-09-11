#include "VulkanAllocator.hpp"

#include "VulkanCheck.hpp"

#include <spdlog/spdlog.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

FVulkanAllocator::FVulkanAllocator(const vk::Instance Instance, const vk::PhysicalDevice Gpu, const vk::Device Device)
{
	VmaVulkanFunctions Functions{};

	// todo: hardcode version for now
	VmaAllocatorCreateInfo AllocatorInfo{
	    .physicalDevice = Gpu, .device = Device, .instance = Instance, .vulkanApiVersion = VK_API_VERSION_1_3};

	VERIFYVULKANRESULT(vmaImportVulkanFunctionsFromVolk(&AllocatorInfo, &Functions));

	AllocatorInfo.pVulkanFunctions = &Functions;

	VERIFYVULKANRESULT(vmaCreateAllocator(&AllocatorInfo, &Allocator));

	spdlog::info("vma allocator created");
}

void FVulkanAllocator::AllocateBuffer()
{
	// TODO: abstract this i should support one AllocateBuffer function
	// it should work for vertex buffer, index buffer, uniform buffers
	// etc
	vk::BufferCreateInfo BufferInfo{};
	BufferInfo.size = {};
	BufferInfo.usage = {};
	BufferInfo.sharingMode = vk::SharingMode::eExclusive;

	VmaAllocationInfo AllocInfo{};
}

FVulkanAllocator::~FVulkanAllocator()
{
	if (Allocator != VK_NULL_HANDLE)
	{
		vmaDestroyAllocator(Allocator);
		Allocator = VK_NULL_HANDLE;
	}
}
