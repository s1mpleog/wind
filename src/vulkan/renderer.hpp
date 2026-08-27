#pragma once

#include "config.hpp"
#include "render_view.hpp"
#include "resources/resource_manager.hpp"
#include "scene/render_object.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/core/configuration.hpp"
#include "vulkan/core/context.hpp"
#include "vulkan/core/swapchain.hpp"
#include "vulkan/frame/frame_context.hpp"
#include "vulkan/graphics/pipeline_manager.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace wind::vulkan {
class Renderer
{
public:
  Renderer(const Renderer&)                        = delete;
  auto operator=(const Renderer&) -> Renderer&     = delete;
  Renderer(Renderer&&) noexcept                    = default;
  auto operator=(Renderer&&) noexcept -> Renderer& = default;

  // two step initialization
  // todo: later make resource and pipeline manager const
  WIND_NODISCARD static auto create(Configuration               cfg,
                                    const platform::Window&     window,
                                    const VulkanContext*        context,
                                    resources::ResourceManager* resource_manager,
                                    graphics::PipelineManager*  pipeline_manager) WIND_NOEXCEPT -> WindResult<Renderer>;

  WIND_NODISCARD auto shutdown() const WIND_NOEXCEPT -> WindResult<void>
  {
    WIND_TRY(m_context->gpu_device.device.waitIdle());
    return {};
  }

  WIND_NODISCARD auto begin(u32 width, u32 height) WIND_NOEXCEPT -> WindResult<void>;
  auto                draw(scene::RenderObject object, RenderView camera_view) WIND_NOEXCEPT -> void;
  auto                end() WIND_NOEXCEPT -> void;

  // internal functions
  auto draw_model(scene::RenderObject object, RenderView camera_view, vk::raii::CommandBuffer& cmd_buffer) WIND_NOEXCEPT -> void;
  auto draw_buffer(scene::RenderObject object, RenderView camera_view, vk::raii::CommandBuffer& cmd_buffer) WIND_NOEXCEPT -> void;

  // can cmd buffer be const ?
  auto setup_viewport(vk::raii::CommandBuffer& cmd_buffer) const WIND_NOEXCEPT -> void;

private:
  Renderer(Configuration               cfg,
           const VulkanContext*        context,
           SwapchainContext            swapchain_context,
           std::vector<FrameContext>   frame_context,
           resources::ResourceManager* resource_manager,
           graphics::PipelineManager*  pipeline_manager)
      : m_config{std::move(cfg)}
      , m_context{context}
      , m_swapchain_context{std::move(swapchain_context)}
      , m_frame_context(std::move(frame_context))
      , m_pipeline_manager{pipeline_manager}
      , m_resource_manager{resource_manager} {};

  Configuration               m_config;
  const VulkanContext*        m_context;
  SwapchainContext            m_swapchain_context;
  std::vector<FrameContext>   m_frame_context;
  graphics::PipelineManager*  m_pipeline_manager;
  resources::ResourceManager* m_resource_manager;
  gpu::AllocatedBuffer        m_test_vertex_buffer;
  resources::ModelHandle      m_test_model{};

  u32 m_current_frame{0};
  u32 m_current_image{0};
};

}  // namespace wind::vulkan
