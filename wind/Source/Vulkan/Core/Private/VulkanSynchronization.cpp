#include "VulkanSynchronization.hpp"

#include "Check.hpp"
#include "Vulkan/Core/Private/VulkanDevice.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"

FVulkanFence::FVulkanFence(FVulkanDevice &InDevice) : State(EState::NotReady), Device(InDevice) {};

FVulkanFence::~FVulkanFence()
{
}

void FVulkanFence::Create(bool bCreateSignaled)
{
	vk::FenceCreateInfo FenceInfo{};

	if (bCreateSignaled)
	{
		FenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
		State = EState::Signaled;
	}

	VERIFYVULKANRESULT_UNWRAP(Fence, Device.GetHandle().createFence(FenceInfo));
	Handle = std::move(Fence);
}

void FVulkanFence::Destroy()
{
	if (Handle != VK_NULL_HANDLE)
	{
		Device.GetHandle().destroyFence(Handle);
	}
}

FVulkanSemaphore::FVulkanSemaphore(FVulkanDevice &InDevice) : Device(InDevice)
{
	VERIFYVULKANRESULT_UNWRAP(Semaphore, Device.GetHandle().createSemaphore(vk::SemaphoreCreateInfo{}));
	Handle = std::move(Semaphore);
}

FVulkanSemaphore::~FVulkanSemaphore()
{
}

void FVulkanSemaphore::Destroy()
{
	if (Handle != VK_NULL_HANDLE)
	{
		Device.GetHandle().destroySemaphore(Handle);
	}
}
