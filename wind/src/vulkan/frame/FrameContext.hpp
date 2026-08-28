#pragma once

#include "Config.hpp"
#include "./Error.hpp"
#include "utils/ExpectedUtil.hpp"
#include "vulkan/vulkan.hpp"
#include <cstdint>
#include <optional>
#include <vulkan/vulkan_core.h>

namespace wind::vulkan {

// maybe use class ?
struct FrameContext
{
  vk::raii::CommandBuffer                graphics_command_buffer{nullptr};
  std::optional<vk::raii::CommandBuffer> transfer_command_buffer;
  vk::raii::Semaphore                    image_available{nullptr};
  vk::raii::Semaphore                    render_finished{nullptr};
  vk::raii::Fence                        in_flight{nullptr};
  vk::raii::Fence                        present_fence{nullptr};

  WIND_NODISCARD auto wait_in_flight_fence(const vk::raii::Device& device) const WIND_NOEXCEPT -> WindResult<void>
  {
    auto result = device.waitForFences(*this->in_flight, vk::True, UINT64_MAX);
    if(result != vk::Result::eSuccess)
      WIND_ERR(WindError::vulkan(ErrorCode::FailedToWaitForFence, result));

    return {};
  };

  WIND_NODISCARD auto wait_present_fence(const vk::raii::Device& device) const WIND_NOEXCEPT -> WindResult<void>
  {
    auto result = device.waitForFences(*this->present_fence, vk::True, UINT64_MAX);
    if(result != vk::Result::eSuccess)
      WIND_ERR(WindError::vulkan(ErrorCode::FailedToWaitForFence, result));

    return {};
  };

  WIND_NODISCARD auto reset_in_flight_fence(const vk::raii::Device& device) const WIND_NOEXCEPT -> WindResult<void>
  {
    WIND_TRY(device.resetFences(*this->in_flight));
    return {};
  }

  WIND_NODISCARD auto reset_present_fence(const vk::raii::Device& device) const WIND_NOEXCEPT -> WindResult<void>
  {
    WIND_TRY(device.resetFences(*this->present_fence));
    return {};
  }

  WIND_NODISCARD auto reset_cmd_buffer() const WIND_NOEXCEPT -> WindResult<void>
  {
    WIND_TRY(this->graphics_command_buffer.reset());
    return {};
  }

  WIND_NODISCARD auto begin() const WIND_NOEXCEPT -> WindResult<void>
  {
    WIND_TRY(graphics_command_buffer.begin(vk::CommandBufferBeginInfo{}));

    return {};
  }

  WIND_NODISCARD auto end() const WIND_NOEXCEPT -> WindResult<void>
  {
    WIND_TRY(graphics_command_buffer.end());

    return {};
  }
};

namespace frame {
WIND_NODISCARD auto create(u32                          frame_count,
                           const vk::raii::Device&      device,
                           const vk::raii::CommandPool& graphics_pool,
                           const vk::raii::CommandPool* transfer_pool = nullptr) WIND_NOEXCEPT
    -> WindResult<std::vector<FrameContext>>;
}

};  // namespace wind::vulkan
