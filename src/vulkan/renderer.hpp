#pragma once

#include "config.hpp"
#include "resources/resource_manager.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/core/configuration.hpp"
#include "vulkan/core/context.hpp"
#include "vulkan/frame/frame_context.hpp"
#include "vulkan/graphics/pipeline_manager.hpp"
#include <memory>

namespace wind::vulkan {

class Renderer
{
public:
  Renderer(const Renderer&)                        = delete;
  auto operator=(const Renderer&) -> Renderer&     = delete;
  Renderer(Renderer&&) noexcept                    = default;
  auto operator=(Renderer&&) noexcept -> Renderer& = default;

  // two step initialization
  WIND_NODISCARD static auto create(Configuration cfg, const platform::Window& window) WIND_NOEXCEPT -> WindResult<Renderer>;
  auto initialize_resources() WIND_NOEXCEPT -> WindResult<void>;

  WIND_NODISCARD auto shutdown() const WIND_NOEXCEPT -> WindResult<void>
  {
    WIND_TRY(m_context->gpu_device.device.waitIdle());
    return {};
  }

  WIND_NODISCARD auto begin(u32 width, u32 height) WIND_NOEXCEPT -> WindResult<void>;
  auto                draw() WIND_NOEXCEPT -> void;
  auto                end() WIND_NOEXCEPT -> void;

private:
  Renderer(Configuration                               cfg,
           std::unique_ptr<VulkanContext>              context,
           std::vector<FrameContext>                   frame_context,
           std::unique_ptr<resources::ResourceManager> resource_manager)
      : m_config{std::move(cfg)}
      , m_context{std::move(context)}
      , m_frame_context(std::move(frame_context))
      , m_resource_manager{std::move(resource_manager)} {};

  Configuration                               m_config;
  std::unique_ptr<VulkanContext>              m_context;
  std::vector<FrameContext>                   m_frame_context;
  graphics::PipelineManager                   m_pipeline_manager;
  vulkan::memory::AllocatedBuffer             m_test_vertex_buffer;
  std::unique_ptr<resources::ResourceManager> m_resource_manager;

  u32 m_current_frame{0};
  u32 m_current_image{0};
};

}  // namespace wind::vulkan
