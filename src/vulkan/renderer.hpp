#pragma once

#include "config.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/core/context.hpp"
#include "vulkan/frame/frame_context.hpp"
#include "vulkan/graphics/pipeline_manager.hpp"

namespace wind::vulkan {

class Renderer
{
public:
  Renderer(const Renderer&)                    = delete;
  auto operator=(const Renderer&) -> Renderer& = delete;

  Renderer(Renderer&&) noexcept                    = default;
  auto operator=(Renderer&&) noexcept -> Renderer& = default;

  WIND_NODISCARD static auto create(Configuration cfg, const platform::Window& window) WIND_NOEXCEPT -> WindResult<Renderer>;

  WIND_NODISCARD auto begin() WIND_NOEXCEPT -> WindResult<void>;

private:
  explicit Renderer(VulkanContext context, std::vector<FrameContext> frame_context)
      : m_context{std::move(context)}
      , m_frame_context(std::move(frame_context)) {};

  VulkanContext             m_context;
  std::vector<FrameContext> m_frame_context;
  graphics::PipelineManager m_pipeline_manager;

  u32 m_current_frame{0};
  u32 m_current_image{0};
};

}  // namespace wind::vulkan
