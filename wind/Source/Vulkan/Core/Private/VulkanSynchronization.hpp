#pragma once

// FVulkanFence Fence{Signaled}
// Fence.wait()
// Fence.reset()
// Fence.destroy()

#include "Config.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"

class FVulkanDevice;

class FVulkanFence
{
  public:
	enum EState : uint8
	{
		NotReady,
		Signaled
	};

	FVulkanFence(FVulkanDevice &InDevice);
	~FVulkanFence();

	FVulkanFence(const FVulkanFence &) = delete;
	FVulkanFence &operator=(const FVulkanFence &) = delete;

	FVulkanFence(FVulkanFence &&Other) noexcept;
	FVulkanFence &operator=(FVulkanFence &&Other) noexcept;

	void Create(bool bCreateSignaled = false);

	bool IsSignaled() const
	{
		return State == EState::Signaled;
	}

	WIND_INLINE vk::Fence GetHandle() const
	{
		return Handle;
	}

	void Destroy();

  private:
	EState State = EState::NotReady;
	FVulkanDevice &Device;
	vk::Fence Handle = VK_NULL_HANDLE;
};

class FVulkanSemaphore
{
  public:
	FVulkanSemaphore(FVulkanDevice &InDevice);
	~FVulkanSemaphore();

	FVulkanSemaphore(const FVulkanSemaphore &) = delete;
	FVulkanSemaphore &operator=(const FVulkanSemaphore &) = delete;

	FVulkanSemaphore(FVulkanSemaphore &&Other) noexcept;
	FVulkanSemaphore &operator=(FVulkanSemaphore &&Other) noexcept;

	WIND_INLINE vk::Semaphore GetHandle() const
	{
		return Handle;
	}

	void Destroy();

  private:
	FVulkanDevice &Device;
	vk::Semaphore Handle = VK_NULL_HANDLE;
};
