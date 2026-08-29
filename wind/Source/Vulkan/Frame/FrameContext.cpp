#include "Vulkan/Frame/FrameContext.hpp"

#include "Error.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "spdlog/spdlog.h"

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

WIND_NODISCARD auto CreateFrame(uint32 FrameCount, const vk::raii::Device &Device,
                                const vk::raii::CommandPool &GraphicsPool,
                                const vk::raii::CommandPool *TransferPool) WIND_NOEXCEPT
    -> TWindResult<std::vector<FFRameContext>>
{
	WIND_ASSERT(FrameCount != 0 && "Frame count is zero");

	std::vector<FFRameContext> FrameContext;
	FrameContext.reserve(FrameCount);

	auto GraphicsCmdBuffer = WIND_TRY(Device.allocateCommandBuffers(
	    vk::CommandBufferAllocateInfo(GraphicsPool, vk::CommandBufferLevel::ePrimary, FrameCount)));

	std::optional<vk::raii::CommandBuffer> TransferCmdBuffer;

	if (TransferPool != nullptr)
	{
		auto Buffers = WIND_TRY(Device.allocateCommandBuffers(
		    vk::CommandBufferAllocateInfo{*TransferPool, vk::CommandBufferLevel::ePrimary, FrameCount}));

		WIND_ENSURE_NOT_EMPTY(Buffers, WindError::vulkan(ErrorCode::FailedToAllocateCommandBuffer));

		TransferCmdBuffer = std::move(Buffers[0]);
	}

	for (size_t I = 0; I < FrameCount; ++I)
	{
		FrameContext.emplace_back(
		    std::move(GraphicsCmdBuffer[I]), std::move(TransferCmdBuffer),
		    WIND_TRY(Device.createSemaphore(vk::SemaphoreCreateInfo{}), ErrorCode::FailedToCreateSemaphore),
		    WIND_TRY(Device.createSemaphore(vk::SemaphoreCreateInfo{}), ErrorCode::FailedToCreateSemaphore),
		    WIND_TRY(Device.createFence(vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled}),
		             ErrorCode::FailedToCreateFence),
		    WIND_TRY(Device.createFence(vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled}),
		             ErrorCode::FailedToCreateFence));
	}

#ifdef WIND_LOG_ENABLE
	spdlog::info("created {} frame context", FrameCount);
#endif

	return FrameContext;
}
