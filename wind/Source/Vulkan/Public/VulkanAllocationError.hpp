#pragma once

#include <expected>
#include <vulkan/vulkan.hpp>

enum class EAllocationError
{
	OutOfDeviceMemory,
	OutOfHostMemory,
	FragmentedPool,
	OutOfPoolMemory,
	TooManyObjects,

	InvalidCreateInfo,
	UnsupportedFormat,
	UnsupportedUsage,
	UnsupportedTiling,

	StagingAllocationFailed,
	CommandBufferAllocationFailed,
	SubmissionFailed,

	Unknown
};

inline EAllocationError TranslateAllocationError(VkResult Result)
{
	switch (Result)
	{
	case VK_ERROR_OUT_OF_DEVICE_MEMORY:
		return EAllocationError::OutOfDeviceMemory;

	case VK_ERROR_OUT_OF_HOST_MEMORY:
		return EAllocationError::OutOfHostMemory;

	case VK_ERROR_FRAGMENTED_POOL:
		return EAllocationError::FragmentedPool;

	case VK_ERROR_OUT_OF_POOL_MEMORY:
		return EAllocationError::OutOfPoolMemory;

	case VK_ERROR_TOO_MANY_OBJECTS:
		return EAllocationError::TooManyObjects;

	case VK_ERROR_FORMAT_NOT_SUPPORTED:
		return EAllocationError::UnsupportedFormat;

	case VK_ERROR_FEATURE_NOT_PRESENT:
		return EAllocationError::UnsupportedUsage;

	case VK_ERROR_INITIALIZATION_FAILED:
		return EAllocationError::InvalidCreateInfo;

	default:
		return EAllocationError::Unknown;
	}
}

inline EAllocationError TranslateAllocationError(vk::Result Result)
{
	return TranslateAllocationError(static_cast<VkResult>(Result));
}

template <typename T>
using TAllocationResult = std::expected<T, EAllocationError>;