#include "frame_context.hpp"
#include "error.hpp"
#include "spdlog/spdlog.h"
#include "utils/expected_util.hpp"
#include "vulkan/vulkan.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace wind::vulkan::frame {

WIND_NODISCARD auto create(u32                          frame_count,
                           const vk::raii::Device&      device,
                           const vk::raii::CommandPool& graphics_pool,
                           const vk::raii::CommandPool* transfer_pool) WIND_NOEXCEPT -> WindResult<std::vector<FrameContext>>
{
  WIND_ASSERT(frame_count != 0 && "Frame count is zero");

  std::vector<FrameContext> frame_context;
  frame_context.reserve(frame_count);

  auto graphics_cmd_buffer = WIND_TRY(device.allocateCommandBuffers(
      vk::CommandBufferAllocateInfo(graphics_pool, vk::CommandBufferLevel::ePrimary, frame_count)));

  std::optional<vk::raii::CommandBuffer> transfer_cmd_buffer;

  if(transfer_pool != nullptr)
  {
    auto buffers = WIND_TRY(device.allocateCommandBuffers(
        vk::CommandBufferAllocateInfo{*transfer_pool, vk::CommandBufferLevel::ePrimary, frame_count}));

    WIND_ENSURE_NOT_EMPTY(buffers, WindError::vulkan(ErrorCode::FailedToAllocateCommandBuffer));

    transfer_cmd_buffer = std::move(buffers[0]);
  }

  for(usize i = 0; i < frame_count; ++i)
  {
    frame_context.emplace_back(
        std::move(graphics_cmd_buffer[i]), std::move(transfer_cmd_buffer),
        WIND_TRY(device.createSemaphore(vk::SemaphoreCreateInfo{}), ErrorCode::FailedToCreateSemaphore),
        WIND_TRY(device.createSemaphore(vk::SemaphoreCreateInfo{}), ErrorCode::FailedToCreateSemaphore),
        WIND_TRY(device.createFence(vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled}), ErrorCode::FailedToCreateFence),
        WIND_TRY(device.createFence(vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled}), ErrorCode::FailedToCreateFence));
  }

#ifdef WIND_LOG_ENABLE
  spdlog::info("created {} frame context", frame_count);
#endif

  return frame_context;
}

};  // namespace wind::vulkan::frame
