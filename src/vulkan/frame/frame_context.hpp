#pragma once

#include <array>
#include <optional>
#include <vulkan/vulkan_core.h>

namespace wind::vulkan {

struct FrameContext
{
  vk::raii::CommandBuffer                graphics_command_buffer{nullptr};
  std::optional<vk::raii::CommandBuffer> transfer_command_buffer;
  vk::raii::Semaphore                    image_available{nullptr};
  vk::raii::Semaphore                    render_finished{nullptr};
  vk::raii::Fence                        in_flight{nullptr};
};

namespace frame {
WIND_NODISCARD auto create(u32                          frame_count,
                           const vk::raii::Device&      device,
                           const vk::raii::CommandPool& graphics_pool,
                           const vk::raii::CommandPool* transfer_pool = nullptr) WIND_NOEXCEPT
    -> WindResult<std::vector<FrameContext>>;
}

};  // namespace wind::vulkan
