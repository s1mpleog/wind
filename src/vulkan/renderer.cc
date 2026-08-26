#include "renderer.hpp"
#include "SDL3/SDL_scancode.h"
#include "camera.hpp"
#include "core/service_locator.hpp"
#include "error.hpp"
#include "input/input_manager.hpp"
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
#include <glm/glm.hpp>

namespace wind::vulkan {
struct Vertex
{
  glm::vec4 position;
  glm::vec4 color;
};

static_assert(sizeof(Vertex) == 32);
static_assert(offsetof(Vertex, position) == 0);
static_assert(offsetof(Vertex, color) == 16);

static std::array<Vertex, 3> vertices{{
    {{0.0F, 0.5F, 0.0F, 1.0F}, {1.0F, 0.0F, 0.0F, 1.0F}},
    {{0.5F, -0.5F, 0.0F, 1.0F}, {0.0F, 1.0F, 0.0F, 1.0F}},
    {{-0.5F, -0.5F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F, 1.0F}},
}};

struct PushConstants
{
  glm::mat4 transform;
  u32       albedo_texture;
};

WIND_NODISCARD auto Renderer::create(Configuration               cfg,
                                     const platform::Window&     window,
                                     const VulkanContext*        context,
                                     resources::ResourceManager* resource_manager,
                                     graphics::PipelineManager*  pipeline_manager) WIND_NOEXCEPT -> WindResult<Renderer>
{
  // auto context = std::make_unique<VulkanContext>(WIND_TRY(create_context(window, cfg)));
  auto [width, heigth] = window.drawable_size();

  auto swapchain_context = WIND_TRY(swapchain::create(cfg, width, heigth, context->surface, context->gpu_device));

  // frame::create does not stores reference of device or graphics pool
  auto frame_context =
      WIND_TRY(frame::create(MAX_FRAME_IN_FLIGHT, context->gpu_device.device, context->gpu_device.graphics_pool));

  // TEMPORARY
  const float aspect_ratio = static_cast<float>(1280) / static_cast<float>(720);

  auto camera = Camera{};
  camera.init_perspective(60.0F, aspect_ratio, 0.1F, 100.0F);

  return Renderer(std::move(cfg), context, std::move(swapchain_context), std::move(frame_context), resource_manager,
                  pipeline_manager, std::move(camera));
}

auto Renderer::initialize_resources() WIND_NOEXCEPT -> WindResult<void>
{
  // create depth image
  WIND_TRY(m_resource_manager->create_depth_image(m_swapchain_context.extent.width, m_swapchain_context.extent.height));

  auto vertex_shader_handle =
      WIND_TRY(m_resource_manager->load_shader(m_context->gpu_device.device, "assets/shaders/triangle.vert.spv"));

  auto fragment_shader_handle =
      WIND_TRY(m_resource_manager->load_shader(m_context->gpu_device.device, "assets/shaders/triangle.frag.spv"));

  // m_test_vertex_buffer = WIND_TRY(m_resource_manager->te(std::as_bytes(std::span{vertices})));

  ShaderInfo vert_info{
      .stage  = ShaderStage::Vertex,
      .module = WIND_TRY(m_resource_manager->get_shader(vertex_shader_handle)),
  };

  ShaderInfo frag_info{
      .stage  = ShaderStage::Fragment,
      .module = WIND_TRY(m_resource_manager->get_shader(fragment_shader_handle)),
  };

  auto graphics_config = graphics::GraphicsConfig{.shader = {vert_info, frag_info},
                                                  .rasterization{
                                                      .cull_mode    = CullMode::Back,
                                                      .polygon_mode = PolygonMode::Fill,
                                                      .front_face   = FrontFace::ClockWise,
                                                      .discard      = false,
                                                  },
                                                  .vertex_input_state{
                                                      .attributes{{
                                                                      .location = 0,
                                                                      .binding  = 0,
                                                                      .format   = VertexFormat::Float4,
                                                                      .offset   = offsetof(Vertex, position),
                                                                  },
                                                                  {
                                                                      .location = 1,
                                                                      .binding  = 0,
                                                                      .format   = VertexFormat::Float4,
                                                                      .offset   = offsetof(Vertex, color),
                                                                  }},
                                                      .bindings{{
                                                          .binding    = 0,
                                                          .stride     = sizeof(Vertex),
                                                          .input_rate = VertexInputRate::Vertex,
                                                      }},
                                                  },
                                                  .input_assembly{.topology = PrimitiveTopology::TriangleList},
                                                  .depth_stencil{
                                                      .depth_test    = true,
                                                      .depth_write   = true,
                                                      .depth_compare = CompareOp::Less,
                                                  },
                                                  .color_blend  = {.enabled = false},
                                                  .color_format = Format::BGRA8_SRGB,
                                                  .depth_format = Format::D32_FLOAT};

  auto pipeline_handle = WIND_TRY(m_pipeline_manager->create(std::move(graphics_config), m_context->gpu_device.device));

  auto suzanne_vert = WIND_TRY(m_resource_manager->load_shader(m_context->gpu_device.device, "assets/shaders/suzanne.vert.spv"));

  auto suzanne_frag = WIND_TRY(m_resource_manager->load_shader(m_context->gpu_device.device, "assets/shaders/suzanne.frag.spv"));

  // m_test_vertex_buffer = WIND_TRY(m_resource_manager->create_vertices(std::as_bytes(std::span{vertices})));

  ShaderInfo suzanne_vert_info{
      .stage  = ShaderStage::Vertex,
      .module = WIND_TRY(m_resource_manager->get_shader(suzanne_vert)),
  };

  ShaderInfo suzanne_frag_info{
      .stage  = ShaderStage::Fragment,
      .module = WIND_TRY(m_resource_manager->get_shader(suzanne_frag)),
  };

  auto suzanne_config = graphics::GraphicsConfig{.shader = {suzanne_vert_info, suzanne_frag_info},

                                                 .rasterization{
                                                     .cull_mode    = CullMode::Back,
                                                     .polygon_mode = PolygonMode::Fill,
                                                     .front_face   = FrontFace::ClockWise,
                                                     .discard      = false,
                                                 },
                                                 .vertex_input_state{
                                                     .attributes{{
                                                                     .location = 0,
                                                                     .binding  = 0,
                                                                     .format   = VertexFormat::Float3,
                                                                     .offset   = 0,
                                                                 },
                                                                 {
                                                                     .location = 1,
                                                                     .binding  = 1,
                                                                     .format   = VertexFormat::Float3,
                                                                     .offset   = 0,
                                                                 },
                                                                 {
                                                                     .location = 2,
                                                                     .binding  = 2,
                                                                     .format   = VertexFormat::Float2,
                                                                     .offset   = 0,
                                                                 }},
                                                     .bindings{{
                                                                   .binding    = 0,
                                                                   .stride     = sizeof(glm::vec3),
                                                                   .input_rate = VertexInputRate::Vertex,
                                                               },
                                                               {
                                                                   .binding    = 1,
                                                                   .stride     = sizeof(glm::vec3),
                                                                   .input_rate = VertexInputRate::Vertex,
                                                               },
                                                               {
                                                                   .binding    = 2,
                                                                   .stride     = sizeof(glm::vec2),
                                                                   .input_rate = VertexInputRate::Vertex,
                                                               }},
                                                 },
                                                 .input_assembly{.topology = PrimitiveTopology::TriangleList},
                                                 .depth_stencil{
                                                     .depth_test    = true,
                                                     .depth_write   = true,
                                                     .depth_compare = CompareOp::Less,
                                                 },
                                                 .color_blend = {.enabled = false},
                                                 .push_constants{{
                                                     .stage_flags = ShaderStage::Vertex | ShaderStage::Fragment,
                                                     .offset      = 0,
                                                     .size        = sizeof(PushConstants),
                                                 }},
                                                 .descriptor_set_layout = *m_resource_manager->get_bindless_descriptor_layout(),
                                                 .color_format = Format::BGRA8_SRGB,
                                                 .depth_format = Format::D32_FLOAT};

  auto suzanne_pipeline_handle = WIND_TRY(m_pipeline_manager->create(std::move(suzanne_config), m_context->gpu_device.device));

  spdlog::info("suzanne pipeline handle: {}", suzanne_pipeline_handle);

  m_resource_manager->destroy_shader(vertex_shader_handle);
  m_resource_manager->destroy_shader(fragment_shader_handle);
  m_resource_manager->destroy_shader(suzanne_vert);
  m_resource_manager->destroy_shader(suzanne_frag);

  m_test_model = WIND_TRY(m_resource_manager->load_model("assets/models/thanos.wind"));

  return {};
}

WIND_NODISCARD auto Renderer::begin(u32 width, u32 height) WIND_NOEXCEPT -> WindResult<void>
{
  // get a frame
  auto* frame = &m_frame_context[m_current_frame];

  // if(core::ServiceLocator::get<input::InputManger>().is_down(SDL_SCANCODE_S))
  // {
  //   spdlog::info("processing S move");
  // }

  // wait for fences (previous frame to complete)
  WIND_TRY(frame->wait_in_flight_fence(m_context->gpu_device.device));
  WIND_TRY(frame->wait_present_fence(m_context->gpu_device.device));

  // acquire the next image index
  auto [swapchain_result, swapchain_image] =
      m_swapchain_context.handle.acquireNextImage(UINT64_MAX, frame->image_available, nullptr);

  // TODO: fix this
  if(swapchain_result == vk::Result::eErrorOutOfDateKHR || swapchain_result == vk::Result::eSuboptimalKHR)
  {
    WIND_TRY(m_context->gpu_device.device.waitIdle());

    auto old_swapchain = std::move(m_swapchain_context);

    auto new_swapchain = WIND_TRY(swapchain::create(m_config, width, height, m_context->surface, m_context->gpu_device));

    m_swapchain_context = std::move(new_swapchain);

    // create new swapchain
    spdlog::info("out of date swapchain");
  }

  if(swapchain_result != vk::Result::eSuccess && swapchain_result != vk::Result::eSuboptimalKHR
     && swapchain_result != vk::Result::eErrorOutOfDateKHR)
    WIND_ERR(WindError::vulkan(ErrorCode::SwapchainSuboptimal, swapchain_result));

  WIND_TRY(frame->reset_in_flight_fence(m_context->gpu_device.device));
  WIND_TRY(frame->reset_present_fence(m_context->gpu_device.device));

  m_current_image = swapchain_image;

  // reset old command buffer
  WIND_TRY(frame->reset_cmd_buffer());

  // begin recording command
  WIND_TRY(frame->begin());

  // create image memory barrier 2 and rendering attachment info

  //TODO: abstract these
  vk::ImageMemoryBarrier2 color_barrier{};
  color_barrier.image                       = m_swapchain_context.images[m_current_image];
  color_barrier.oldLayout                   = vk::ImageLayout::eUndefined;
  color_barrier.newLayout                   = vk::ImageLayout::eColorAttachmentOptimal;
  color_barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  color_barrier.subresourceRange.levelCount = 1;
  color_barrier.subresourceRange.layerCount = 1;
  color_barrier.dstStageMask                = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
  color_barrier.dstAccessMask               = vk::AccessFlagBits2::eColorAttachmentWrite;
  color_barrier.srcStageMask                = vk::PipelineStageFlagBits2::eTopOfPipe;
  color_barrier.srcAccessMask               = vk::AccessFlagBits2::eNone;

  std::array<float, 4> clear_color{0.055F, 0.055F, 0.055F, 1.0F};

  vk::ClearColorValue color{};
  color.setFloat32(clear_color);

  vk::RenderingAttachmentInfo color_attach_info{};
  color_attach_info.imageLayout      = vk::ImageLayout::eColorAttachmentOptimal;
  color_attach_info.imageView        = m_swapchain_context.image_views[m_current_image];
  color_attach_info.loadOp           = vk::AttachmentLoadOp::eClear;
  color_attach_info.storeOp          = vk::AttachmentStoreOp::eStore;
  color_attach_info.clearValue.color = color;

  vk::RenderingAttachmentInfo depth_attach_info{};
  depth_attach_info.imageView   = m_resource_manager->get_depth_image_view();
  depth_attach_info.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
  depth_attach_info.loadOp      = vk::AttachmentLoadOp::eClear;
  depth_attach_info.storeOp     = vk::AttachmentStoreOp::eDontCare;
  depth_attach_info.clearValue.depthStencil.setDepth(1.0F);

  vk::Rect2D render_area{0, m_swapchain_context.extent};

  vk::RenderingInfo rendering_info{};
  rendering_info.colorAttachmentCount = 1;
  rendering_info.pColorAttachments    = &color_attach_info;
  rendering_info.pDepthAttachment     = &depth_attach_info;
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
  m_camera.update();

  vk::Rect2D scissor{0};
  scissor.extent = m_swapchain_context.extent;

  vk::Viewport viewport{};
  viewport.x     = 0.0F;
  viewport.y     = static_cast<float>(m_swapchain_context.extent.height);
  viewport.width = static_cast<float>(m_swapchain_context.extent.width);
  // upside down triangle fix
  viewport.height   = -static_cast<float>(m_swapchain_context.extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  auto* frame = &m_frame_context[m_current_frame];

  frame->graphics_command_buffer.setViewport(0, viewport);
  frame->graphics_command_buffer.setScissor(0, scissor);

  auto pipeline         = m_pipeline_manager->get(0);
  auto suzanne_pipeline = m_pipeline_manager->get(1);

  std::array pipelines = {*pipeline.value()->graphics_pipeline, *suzanne_pipeline.value()->graphics_pipeline};

  for(const auto& pipeline : pipelines)
  {
    frame->graphics_command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
  }

  // Temporary camera/object transform.
  const glm::mat4 identity{1.0F};

  const glm::mat4 transform = m_camera.view_projection() * identity;

  const auto* model    = m_resource_manager->get_model_unchecked(m_test_model);
  const auto& mesh     = model->mesh;
  const auto& material = model->materials[0];

  PushConstants pc{
      .transform      = transform,
      .albedo_texture = material.albedo_texture,
  };

  frame->graphics_command_buffer.pushConstants(*suzanne_pipeline.value()->pipeline_layout,
                                               vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0,
                                               sizeof(PushConstants), &pc);

  const auto& descriptor_set = *m_resource_manager->get_bindless_descriptor_set();

  frame->graphics_command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                                    *suzanne_pipeline.value()->pipeline_layout, 0, *descriptor_set, {});

  std::array<vk::Buffer, 3> vertex_buffers{mesh.vertex_buffer.buffer, mesh.normals.buffer, mesh.uvs.buffer};

  std::array<vk::DeviceSize, 3> offsets{0, 0, 0};

  frame->graphics_command_buffer.bindVertexBuffers(0, vertex_buffers, offsets);

  frame->graphics_command_buffer.bindIndexBuffer(mesh.index_buffer.buffer, 0, vk::IndexType::eUint32);

  frame->graphics_command_buffer.drawIndexed(mesh.index_count, 1, 0, 0, 0);

  // frame->graphics_command_buffer.draw(3, 1, 0, 0);
}

auto Renderer::end() WIND_NOEXCEPT -> void
{
  auto* frame = &m_frame_context[m_current_frame];

  frame->graphics_command_buffer.endRendering();

  vk::ImageMemoryBarrier2 barrier{};
  barrier.oldLayout     = vk::ImageLayout::eColorAttachmentOptimal;
  barrier.newLayout     = vk::ImageLayout::ePresentSrcKHR;
  barrier.image         = m_swapchain_context.images[m_current_image];
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

  if(auto result = m_context->gpu_device.graphics_queue.submit2(submit_info, frame->in_flight); !result.has_value())
    spdlog::info("Failed to submit queue");

  auto swapchain_handle = *m_swapchain_context.handle;

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

  if(m_context->gpu_device.presentation_queue.presentKHR(present_info) != vk::Result::eSuccess)
    spdlog::info("Failed to present");

  m_current_frame = (m_current_frame + 1) % MAX_FRAME_IN_FLIGHT;
}

}  // namespace wind::vulkan
