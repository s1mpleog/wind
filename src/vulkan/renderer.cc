#include "renderer.hpp"
#include "error.hpp"
#include "platform/window.hpp"
#include "resources/resource_manager.hpp"
#include "spdlog/spdlog.h"
#include "utils/expected_util.hpp"
#include "vulkan/core/configuration.hpp"
#include "vulkan/core/context.hpp"
#include "vulkan/frame/frame_context.hpp"
#include "vulkan/graphics/pipeline_config.hpp"
#include "vulkan/graphics/pipeline_manager.hpp"
#include "vulkan/vulkan.hpp"
#include <array>
#include <cstdint>
#include <vector>

namespace wind::vulkan {

WIND_NODISCARD auto Renderer::create(Configuration cfg, const platform::Window& window) WIND_NOEXCEPT -> WindResult<Renderer>
{
  auto context = WIND_TRY(create_context(window, std::move(cfg)));

  // frame::create does not stores reference of device or graphics pool
  auto frame_context = WIND_TRY(frame::create(MAX_FRAME_IN_FLIGHT, context.device.handle, context.device.graphics_pool));

  auto resource_manager = ResourceManager{};

  auto vertex_shader_handle = WIND_TRY(resource_manager.load_shader(context.device.handle, "assets/shaders/triangle.vert.spv"));

  auto fragment_shader_handle = WIND_TRY(resource_manager.load_shader(context.device.handle, "assets/shaders/triangle.frag.spv"));

  ShaderInfo vert_info{
      .stage  = ShaderStage::Vertex,
      .module = WIND_TRY(resource_manager.get_shader(vertex_shader_handle)),
  };

  ShaderInfo frag_info{
      .stage  = ShaderStage::Fragment,
      .module = WIND_TRY(resource_manager.get_shader(fragment_shader_handle)),
  };

  auto pipeline_manager = graphics::PipelineManager{};

  auto graphics_config = graphics::GraphicsConfig{.shader = {std::move(vert_info), std::move(frag_info)},
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
                                                  .depth_stencil{
                                                      .depth_test = false,
                                                  },
                                                  .color_format = Format::RGBA8_SRGB};

  auto pipeline_handle = WIND_TRY(pipeline_manager.create(std::move(graphics_config), context.device.handle));

  spdlog::info("pipeline handle: {}", pipeline_handle);

  return Renderer(std::move(context), std::move(frame_context));
}

WIND_NODISCARD auto Renderer::begin() WIND_NOEXCEPT -> WindResult<void>
{
  // get a frame
  auto* frame = &m_frame_context[m_current_frame];

  // wait for fences (previous frame to complete)
  WIND_TRY(frame->wait(m_context.device.handle));

  // acquire the next image index
  auto [swapchain_result, swapchain_image] =
      m_context.swapchain.handle.acquireNextImage(UINT64_MAX, frame->image_available, nullptr);

  if(swapchain_result == vk::Result::eErrorOutOfDateKHR)
  {
    // create new swapchain
    spdlog::info("out of date swapchain");
  }

  if(swapchain_result != vk::Result::eSuccess && swapchain_result != vk::Result::eSuboptimalKHR)
    WIND_ERR(WindError::vulkan(ErrorCode::SwapchainSuboptimal, swapchain_result));

  WIND_TRY(frame->reset_fence(m_context.device.handle));

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

  std::array<float, 4> clear_color{0.5F, 0.2F, 1.0F, 1.0F};

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

}  // namespace wind::vulkan
