#pragma once

#include "Config.hpp"
#include "Error.hpp"
#include "Utils/ExpectedUtil.hpp"

#include <cstdint>
#include <optional>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

// maybe use class ?
struct FRameContext
{
	vk::raii::CommandBuffer GraphicsCommandBuffer{nullptr};
	std::optional<vk::raii::CommandBuffer> TransferCommandBuffer;
	vk::raii::Semaphore ImageAvailable{nullptr};
	vk::raii::Semaphore RenderFinished{nullptr};
	vk::raii::Fence InFlight{nullptr};
	vk::raii::Fence PresentFence{nullptr};

	WIND_NODISCARD auto WaitInFlightFence(const vk::raii::Device &Device) const WIND_NOEXCEPT -> TWindResult<void>
	{
		auto Result = Device.waitForFences(*this->InFlight, vk::True, UINT64_MAX);
		if (Result != vk::Result::eSuccess)
			WIND_ERR(WindError::vulkan(ErrorCode::FailedToWaitForFence, Result));

		return {};
	};

	WIND_NODISCARD auto WaitPresentFence(const vk::raii::Device &Device) const WIND_NOEXCEPT -> TWindResult<void>
	{
		auto Result = Device.waitForFences(*this->PresentFence, vk::True, UINT64_MAX);
		if (Result != vk::Result::eSuccess)
			WIND_ERR(WindError::vulkan(ErrorCode::FailedToWaitForFence, Result));

		return {};
	};

	WIND_NODISCARD auto ResetInFlightFence(const vk::raii::Device &Device) const WIND_NOEXCEPT -> TWindResult<void>
	{
		WIND_TRY(Device.resetFences(*this->InFlight));
		return {};
	}

	WIND_NODISCARD auto ResetPresentFence(const vk::raii::Device &Device) const WIND_NOEXCEPT -> TWindResult<void>
	{
		WIND_TRY(Device.resetFences(*this->PresentFence));
		return {};
	}

	WIND_NODISCARD auto ResetCmdBuffer() const WIND_NOEXCEPT -> TWindResult<void>
	{
		WIND_TRY(this->GraphicsCommandBuffer.reset());
		return {};
	}

	WIND_NODISCARD auto Begin() const WIND_NOEXCEPT -> TWindResult<void>
	{
		WIND_TRY(GraphicsCommandBuffer.begin(vk::CommandBufferBeginInfo{}));

		return {};
	}

	WIND_NODISCARD auto End() const WIND_NOEXCEPT -> TWindResult<void>
	{
		WIND_TRY(GraphicsCommandBuffer.end());

		return {};
	}
};

WIND_NODISCARD auto CreateFrame(TU32 FrameCount, const vk::raii::Device &Device,
                                const vk::raii::CommandPool &GraphicsPool,
                                const vk::raii::CommandPool *TransferPool = nullptr) WIND_NOEXCEPT
    -> TWindResult<std::vector<FRameContext>>;
