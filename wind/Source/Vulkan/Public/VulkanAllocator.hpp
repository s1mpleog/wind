#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

class FVulkanAllocator
{
  public:
	FVulkanAllocator() = default;
	FVulkanAllocator(const vk::Instance Instance, const vk::PhysicalDevice Gpu, const vk::Device Device);
	~FVulkanAllocator();

	void AllocateBuffer();

  private:
	VmaAllocator Allocator = VK_NULL_HANDLE;
};
