#include "renderer.hpp"
#include "error.hpp"
#include "platform/window.hpp"
#include "resources/resource_manager.hpp"
#include "spdlog/spdlog.h"
#include "utils/expected_util.hpp"
#include "vulkan/core/configuration.hpp"
#include "vulkan/core/context.hpp"
#include "vulkan/core/swapchain.hpp"
#include "vulkan/frame/frame_context.hpp"
#include "vulkan/graphics/pipeline_config.hpp"
#include "vulkan/graphics/pipeline_manager.hpp"
#include "vulkan/vulkan.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace wind::vulkan {
// struct Vertex
// {
//   glm::vec4 position;
//   glm::vec4 color;
// };

// static_assert(sizeof(Vertex) == 32);
// static_assert(offsetof(Vertex, position) == 0);
// static_assert(offsetof(Vertex, color) == 16);

// static std::array<Vertex, 3> vertices{{
//     {{0.0f, -0.5f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
//     {{0.5f, 0.5f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
//     {{-0.5f, 0.5f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
// }};

WIND_NODISCARD auto Renderer::create(Configuration cfg, const platform::Window& window) WIND_NOEXCEPT -> WindResult<Renderer>
{
  auto context = WIND_TRY(create_context(window, std::move(cfg)));

  // frame::create does not stores reference of device or graphics pool
  auto frame_context = WIND_TRY(frame::create(MAX_FRAME_IN_FLIGHT, context.gpu_device.device, context.gpu_device.graphics_pool));

  auto resource_manager = WIND_TRY(resources::ResourceManager::create(context));

  auto vertex_shader_handle =
      WIND_TRY(resource_manager.load_shader(context.gpu_device.device, "assets/shaders/triangle.vert.spv"));

  auto fragment_shader_handle =
      WIND_TRY(resource_manager.load_shader(context.gpu_device.device, "assets/shaders/triangle.frag.spv"));

  ShaderInfo vert_info{
      .stage  = ShaderStage::Vertex,
      .module = WIND_TRY(resource_manager.get_shader(vertex_shader_handle)),
  };

  ShaderInfo frag_info{
      .stage  = ShaderStage::Fragment,
      .module = WIND_TRY(resource_manager.get_shader(fragment_shader_handle)),
  };

  auto pipeline_manager = graphics::PipelineManager{};

  auto graphics_config = graphics::GraphicsConfig{.shader = {vert_info, frag_info},
                                                  .rasterization{
                                                      .cull_mode    = CullMode::Back,
                                                      .polygon_mode = PolygonMode::Fill,
                                                      .front_face   = FrontFace::CounterClockwise,
                                                      .discard      = false,
                                                  },
                                                  .vertex_input_state{
                                                      .attributes{},
                                                      .bindings{},
                                                  },
                                                  .input_assembly{.topology = PrimitiveTopology::TriangleList},
                                                  .depth_stencil{
                                                      .depth_test = false,
                                                  },
                                                  .color_blend  = ColorBlendState::opaque(),
                                                  .color_format = Format::BGRA8_SRGB};

  auto pipeline_handle = WIND_TRY(pipeline_manager.create(std::move(graphics_config), context.gpu_device.device));

  spdlog::info("pipeline handle: {}", pipeline_handle);

  resource_manager.destroy_shader(vertex_shader_handle);
  resource_manager.destroy_shader(fragment_shader_handle);

  {
    auto mesh = WIND_TRY(resource_manager.load_asset("assets/models/chair.wind"));
  }

  return Renderer(std::move(cfg), std::move(context), std::move(frame_context), std::move(pipeline_manager));
}

WIND_NODISCARD auto Renderer::begin(u32 width, u32 height) WIND_NOEXCEPT -> WindResult<void>
{
  // get a frame
  auto* frame = &m_frame_context[m_current_frame];

  // wait for fences (previous frame to complete)
  WIND_TRY(frame->wait_in_flight_fence(m_context.gpu_device.device));
  WIND_TRY(frame->wait_present_fence(m_context.gpu_device.device));

  // acquire the next image index
  auto [swapchain_result, swapchain_image] =
      m_context.swapchain.handle.acquireNextImage(UINT64_MAX, frame->image_available, nullptr);

  // TODO: fix this
  if(swapchain_result == vk::Result::eErrorOutOfDateKHR || swapchain_result == vk::Result::eSuboptimalKHR)
  {
    WIND_TRY(m_context.gpu_device.device.waitIdle());

    auto old_swapchain = std::move(m_context.swapchain);

    auto new_swapchain = WIND_TRY(swapchain::create(m_config, width, height, m_context.surface, m_context.gpu_device));

    m_context.swapchain = std::move(new_swapchain);

    // create new swapchain
    spdlog::info("out of date swapchain");
  }

  if(swapchain_result != vk::Result::eSuccess && swapchain_result != vk::Result::eSuboptimalKHR
     && swapchain_result != vk::Result::eErrorOutOfDateKHR)
    WIND_ERR(WindError::vulkan(ErrorCode::SwapchainSuboptimal, swapchain_result));

  WIND_TRY(frame->reset_in_flight_fence(m_context.gpu_device.device));
  WIND_TRY(frame->reset_present_fence(m_context.gpu_device.device));

  m_current_image = swapchain_image;

  // reset old command buffer
  WIND_TRY(frame->reset_cmd_buffer());

  // begin recording command
  WIND_TRY(frame->begin());

  // create image memory barrier 2 and rendering attachment info

  //TODO: abstract these
  vk::ImageMemoryBarrier2 color_barrier{};
  color_barrier.image                       = m_context.swapchain.images[m_current_image];
  color_barrier.oldLayout                   = vk::ImageLayout::eUndefined;
  color_barrier.newLayout                   = vk::ImageLayout::eColorAttachmentOptimal;
  color_barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  color_barrier.subresourceRange.levelCount = 1;
  color_barrier.subresourceRange.layerCount = 1;
  color_barrier.dstStageMask                = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
  color_barrier.dstAccessMask               = vk::AccessFlagBits2::eColorAttachmentWrite;
  color_barrier.srcStageMask                = vk::PipelineStageFlagBits2::eTopOfPipe;
  color_barrier.srcAccessMask               = vk::AccessFlagBits2::eNone;

  std::array<float, 4> clear_color{0.5F, 0.2F, 0.2F, 0.2F};

  vk::ClearColorValue color{};
  color.setFloat32(clear_color);

  vk::RenderingAttachmentInfo color_attach_info{};
  color_attach_info.imageLayout      = vk::ImageLayout::eColorAttachmentOptimal;
  color_attach_info.imageView        = m_context.swapchain.image_views[m_current_image];
  color_attach_info.loadOp           = vk::AttachmentLoadOp::eClear;
  color_attach_info.storeOp          = vk::AttachmentStoreOp::eStore;
  color_attach_info.clearValue.color = color;

  vk::Rect2D render_area{0, m_context.swapchain.extent};

  vk::RenderingInfo rendering_info{};
  rendering_info.colorAttachmentCount = 1;
  rendering_info.pColorAttachments    = &color_attach_info;
  rendering_info.renderArea           = render_area;
  rendering_info.layerCount           = 1;

  vk::DependencyInfo dep_info{};
  dep_info.imageMemoryBarrierCount = 1;
  dep_info.pImageMemoryBarriers    = &color_barrier;

  frame->graphics_command_buffer.pipelineBarrier2(dep_info);

  frame->graphics_command_buffer.beginRendering(rendering_info);

  return {};
}

auto Renderer::draw() WIND_NOEXCEPT -> void
{
  vk::Rect2D scissor{0};
  scissor.extent = m_context.swapchain.extent;

  vk::Viewport viewport{};
  viewport.x     = 0.0f;
  viewport.y     = static_cast<float>(m_context.swapchain.extent.height);
  viewport.width = static_cast<float>(m_context.swapchain.extent.width);
  // upside down triangle fix
  viewport.height   = -static_cast<float>(m_context.swapchain.extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  auto* frame = &m_frame_context[m_current_frame];

  frame->graphics_command_buffer.setViewport(0, viewport);
  frame->graphics_command_buffer.setScissor(0, scissor);

  auto pipeline = m_pipeline_manager.get(0);

  if(!pipeline.has_value())
  {
    spdlog::info("failed to get pipeline");
  }

  frame->graphics_command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline.value()->graphics_pipeline);

  frame->graphics_command_buffer.draw(3, 1, 0, 0);
}

auto Renderer::end() WIND_NOEXCEPT -> void
{
  auto* frame = &m_frame_context[m_current_frame];

  frame->graphics_command_buffer.endRendering();

  vk::ImageMemoryBarrier2 barrier{};
  barrier.oldLayout     = vk::ImageLayout::eColorAttachmentOptimal;
  barrier.newLayout     = vk::ImageLayout::ePresentSrcKHR;
  barrier.image         = m_context.swapchain.images[m_current_image];
  barrier.srcStageMask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
  barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
  barrier.dstStageMask  = vk::PipelineStageFlagBits2::eNone;
  barrier.dstAccessMask = vk::AccessFlagBits2::eNone;

  barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.layerCount = 1;

  barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  vk::DependencyInfo dep_info{};
  dep_info.imageMemoryBarrierCount = 1;
  dep_info.pImageMemoryBarriers    = &barrier;

  frame->graphics_command_buffer.pipelineBarrier2(dep_info);

  if(!frame->end())
    return;

  vk::CommandBufferSubmitInfo cmd_buffer_submit_info{};
  cmd_buffer_submit_info.commandBuffer = frame->graphics_command_buffer;

  vk::SemaphoreSubmitInfo wait_semaphore_info{};
  wait_semaphore_info.semaphore = *frame->image_available;
  wait_semaphore_info.stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;

  vk::SemaphoreSubmitInfo render_finished_semaphore_info{};
  render_finished_semaphore_info.semaphore = *frame->render_finished;
  render_finished_semaphore_info.stageMask = vk::PipelineStageFlagBits2::eAllGraphics;

  vk::SubmitInfo2 submit_info{};

  submit_info.commandBufferInfoCount = 1;
  submit_info.pCommandBufferInfos    = &cmd_buffer_submit_info;

  submit_info.waitSemaphoreInfoCount = 1;
  submit_info.pWaitSemaphoreInfos    = &wait_semaphore_info;

  submit_info.signalSemaphoreInfoCount = 1;
  submit_info.pSignalSemaphoreInfos    = &render_finished_semaphore_info;

  if(auto result = m_context.gpu_device.graphics_queue.submit2(submit_info, frame->in_flight); !result.has_value())
    spdlog::info("Failed to submit queue");

  auto swapchain_handle = *m_context.swapchain.handle;

  vk::SwapchainPresentFenceInfoKHR present_fence_info{};
  present_fence_info.swapchainCount = 1;
  present_fence_info.pFences        = &*frame->present_fence;

  vk::PresentInfoKHR present_info{};
  present_info.pNext              = &present_fence_info;
  present_info.swapchainCount     = 1;
  present_info.pSwapchains        = &swapchain_handle;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores    = &*frame->render_finished;
  present_info.pImageIndices      = &m_current_image;

  if(m_context.gpu_device.presentation_queue.presentKHR(present_info) != vk::Result::eSuccess)
    spdlog::info("Failed to present");

  m_current_frame = (m_current_frame + 1) % MAX_FRAME_IN_FLIGHT;
}

}  // namespace wind::vulkan
