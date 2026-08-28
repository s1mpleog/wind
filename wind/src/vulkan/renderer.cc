#include "renderer.hpp"
#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_to_string.hpp>
#include <array>
#include <cstdint>
#include <vector>
#include "config.hpp"
#include "error.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/trigonometric.hpp"
#include "platform/window.hpp"
#include "resources/resource_manager.hpp"
#include "spdlog/spdlog.h"
#include "utils/expected_util.hpp"
#include "vulkan/core/configuration.hpp"
#include "vulkan/core/context.hpp"
#include "vulkan/core/swapchain.hpp"
#include "vulkan/core/synchroization.hpp"
#include "vulkan/frame/frame_context.hpp"
#include "vulkan/graphics/pipeline_manager.hpp"
#include "vulkan/graphics/shader_types.hpp"
#include "vulkan/vulkan.hpp"

namespace wind::vulkan {
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

  // create depth image
  WIND_TRY(resource_manager->create_default_depth_image(swapchain_context.extent.width, swapchain_context.extent.height));

  // auto ubo_handle = WIND_TRY(resource_manager->create_dynamic_uniform_buffer(sizeof(UboInstance)));

  return Renderer(std::move(cfg), context, std::move(swapchain_context), std::move(frame_context), resource_manager,
                  pipeline_manager, resources::DynamicBufferHandle{.index = 0});
}

WIND_NODISCARD auto Renderer::begin(u32 width, u32 height) WIND_NOEXCEPT -> WindResult<void>
{
  // get a frame
  auto* frame = &m_frame_context[m_current_frame];

  // wait for fences (previous frame to complete)
  // vk::WaitForFences will return once fence is in signaled state so like once GPU processed the previous
  // frame it will set the fence to signaled and this will return
  WIND_TRY(frame->wait_in_flight_fence(m_context->gpu_device.device));
  WIND_TRY(frame->wait_present_fence(m_context->gpu_device.device));

  uint32_t image_index{0};

  // we are using c-api here because c++ library is acting weird here its returning error
  // ask presentation engine for next swapchain image handle once this function success then image_available semaphore
  // will be SIGNALED
  VkResult raw_result = m_context->gpu_device.device.getDispatcher()->vkAcquireNextImageKHR(
      *m_context->gpu_device.device, *m_swapchain_context.handle, UINT64_MAX, *frame->image_available, VK_NULL_HANDLE, &image_index);

  auto swapchain_result = static_cast<vk::Result>(raw_result);

  if(swapchain_result == vk::Result::eErrorOutOfDateKHR)
  {
    // no image was acquired recreate swapchain acquire again
    // stop the current frame

    // TODO: change this with different option
    WIND_TRY(m_context->gpu_device.device.waitIdle());

    spdlog::info("out of date swapchain recreating");

    auto old_swapchain = std::move(m_swapchain_context);
    auto new_swapchain =
        WIND_TRY(swapchain::create(m_config, width, height, m_context->surface, m_context->gpu_device, &old_swapchain.handle));

    m_swapchain_context = std::move(new_swapchain);

    // recreate the depth buffer
    WIND_TRY(m_resource_manager->create_default_depth_image(width, height));

    // this does not means error i am doing this so in draw call i check this error code
    // if error == out_of_date then continue otherwise return error
    WIND_ERR(WindError::vulkan(ErrorCode::SwapchainOutOfDate, swapchain_result));
  }

  if(swapchain_result != vk::Result::eSuccess && swapchain_result != vk::Result::eSuboptimalKHR
     && swapchain_result != vk::Result::eErrorOutOfDateKHR)
  {
    WIND_ERR(WindError::vulkan(ErrorCode::SwapchainSuboptimal, swapchain_result));
  }

  // vk::ResetFence the specs says when vk::ResetFence is executed on host (CPU), it defines
  // a fence un-signaled operation for each fence, which resets the fence to un-signaled state
  // we need this because vulkan states that we must not submit to queue with signaled fence
  // so we have to do it like if we submit to signaled fence then when cpu waits it will return
  // automatically since it is in signaled fence let GPU change from un-signaled to signaled when it
  // finishes the operation
  WIND_TRY(frame->reset_in_flight_fence(m_context->gpu_device.device));
  WIND_TRY(frame->reset_present_fence(m_context->gpu_device.device));

  // set the current image index to what swapchain index gave us
  m_current_image = image_index;

  // reset old command buffer
  // the specs says any state other than pending can be transitioned to initial state by
  // called vk::ResetCommandBuffer pending = when we do queue submit the specs also says application
  // must not modify command buffer which is in pending state
  WIND_TRY(frame->reset_cmd_buffer());

  // begin recording command
  // set the command buffer state from initial to recording state by calling vk::BeginCommandBuffer
  // the specs says recording state can be used to record commands via vk::Cmd*
  WIND_TRY(frame->begin());

  // transition swapchain image from undefined to optimal for color attachment
  // we can't use undefined layout for color attachment aka rendering we must transition first
  sync::transition_image(frame->graphics_command_buffer, m_swapchain_context.images[m_current_image],
                         vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);

  std::array<float, 4> clear_color{0.055F, 0.055F, 0.055F, 1.0F};

  vk::ClearColorValue color{};
  color.setFloat32(clear_color);

  vk::RenderingAttachmentInfo color_attach_info{};
  color_attach_info.imageLayout      = vk::ImageLayout::eColorAttachmentOptimal;
  color_attach_info.imageView        = m_swapchain_context.image_views[m_current_image];
  color_attach_info.loadOp           = vk::AttachmentLoadOp::eClear;
  color_attach_info.storeOp          = vk::AttachmentStoreOp::eStore;
  color_attach_info.clearValue.color = color;

  // create depth attachment
  vk::RenderingAttachmentInfo depth_attach_info{};
  depth_attach_info.imageView   = m_resource_manager->get_default_depth_image_view();
  depth_attach_info.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
  depth_attach_info.loadOp      = vk::AttachmentLoadOp::eClear;
  depth_attach_info.storeOp     = vk::AttachmentStoreOp::eDontCare;
  depth_attach_info.clearValue.depthStencil.setDepth(1.0F);

  vk::Rect2D render_area{0, m_swapchain_context.extent};

  // information for dynamic rendering
  vk::RenderingInfo rendering_info{};
  rendering_info.colorAttachmentCount = 1;
  rendering_info.pColorAttachments    = &color_attach_info;
  rendering_info.pDepthAttachment     = &depth_attach_info;
  rendering_info.renderArea           = render_area;
  rendering_info.layerCount           = 1;

  // begin a dynamic render pass instance the specs says after beginning a render pass instance
  // the command buffer is ready to record draw commands
  frame->graphics_command_buffer.beginRendering(rendering_info);

  return {};
}

auto Renderer::draw_buffer(scene::RenderObject object, RenderView camera_view, vk::raii::CommandBuffer& cmd_buffer) WIND_NOEXCEPT -> void
{
  WIND_ASSERT(!object.is_model_type && "Trying to draw buffer but object is model");

  const auto* pipeline = m_pipeline_manager->get_unchecked(object.pipeline_handle);
  cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->graphics_pipeline);

  const auto* vertex_buffer = m_resource_manager->get_buffer_unchecked(object.buffer_asset.vertex_handle);
  const auto* index_buffer  = m_resource_manager->get_buffer_unchecked(object.buffer_asset.index_handle);

  auto push_constant = PushConstants{.transform      = camera_view.projection * camera_view.view
                                                       * glm::translate(glm::mat4{1.0F}, glm::vec3{0.0F, 0.0F, -10.0F}),
                                     .albedo_texture = 0,
                                     .normal_index   = 0,
                                     .metallic_roughness_index = 0,
                                     .base_color               = glm::vec4{0.0F, 0.0F, 0.0F, 0.0F}};

  cmd_buffer.pushConstants(pipeline->pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants), &push_constant);

  std::array<vk::Buffer, 1>     buffers{vertex_buffer->buffer};
  std::array<vk::DeviceSize, 1> offsets{0};

  cmd_buffer.bindVertexBuffers(0, buffers, offsets);
  cmd_buffer.bindIndexBuffer(index_buffer->buffer, 0, vk::IndexType::eUint16);

  cmd_buffer.drawIndexed(object.buffer_asset.index_count, 1, 0, 0, 0);
}

auto Renderer::draw_model(scene::RenderObject object, RenderView camera_view, vk::raii::CommandBuffer& cmd_buffer) WIND_NOEXCEPT -> void
{
  WIND_ASSERT(object.is_model_type && "Trying to draw model but object is not model");

  const auto* pipeline = m_pipeline_manager->get_unchecked(object.pipeline_handle);

  cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->graphics_pipeline);

  const auto* model = m_resource_manager->get_model_unchecked(object.model_handle);
  const auto& mesh  = model->mesh;

  const auto& descriptor_set = *m_resource_manager->get_bindless_descriptor_set();

  // auto  ubo_instance = UboInstance{.transform = camera_view.projection * camera_view.view * glm::mat4{1.0F}};
  // auto* mapped       = m_resource_manager->get_mapped_data_unchecked(m_frame_ubo);
  // std::memcpy(mapped, &ubo_instance, sizeof(ubo_instance));

  cmd_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline->pipeline_layout, 0, *descriptor_set, {0});

  std::array<vk::Buffer, 4> buffers{mesh.vertex_buffer.buffer, mesh.normals.buffer, mesh.uvs.buffer, mesh.tangents.buffer};
  std::array<vk::DeviceSize, 4> offsets{0, 0, 0, 0};

  cmd_buffer.bindVertexBuffers(0, buffers, offsets);
  cmd_buffer.bindIndexBuffer(mesh.index_buffer.buffer, 0, vk::IndexType::eUint32);

  for(const auto& submesh : mesh.sub_meshes)
  {
    const auto& material  = model->materials[submesh.material_index];
    auto        transform = camera_view.projection * camera_view.view
                            * glm::rotate(glm::mat4{1.0F}, glm::radians(45.0F), glm::vec3{0.0F, 1.0F, 0.0F});

    PushConstants pc{
        .transform      = transform,
        .albedo_texture = material.albedo_texture ? material.albedo_texture.value() : UINT32_MAX,
        .normal_index   = material.normal_texture ? material.normal_texture.value() : UINT32_MAX,
        .metallic_roughness_index = material.metallic_roughness_texture ? material.metallic_roughness_texture.value() : UINT32_MAX,
        .base_color = material.base_color,
    };

    cmd_buffer.pushConstants(pipeline->pipeline_layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                             0, sizeof(PushConstants), &pc);

    cmd_buffer.drawIndexed(submesh.index_count, 1, submesh.index_offset, 0, 0);
  }
}

auto Renderer::setup_viewport(vk::raii::CommandBuffer& cmd_buffer) const WIND_NOEXCEPT -> void
{
  vk::Rect2D scissor{0};
  scissor.extent = m_swapchain_context.extent;

  vk::Viewport viewport{};
  viewport.x        = 0.0F;
  viewport.y        = 0.0F;
  viewport.width    = static_cast<float>(m_swapchain_context.extent.width);
  viewport.height   = static_cast<float>(m_swapchain_context.extent.height);
  viewport.minDepth = 0.0F;
  viewport.maxDepth = 1.0F;

  cmd_buffer.setViewport(0, viewport);
  cmd_buffer.setScissor(0, scissor);
}

auto Renderer::draw(scene::RenderObject object, RenderView camera_view) WIND_NOEXCEPT -> void
{
  auto* frame = &m_frame_context[m_current_frame];

  setup_viewport(frame->graphics_command_buffer);

  if(object.is_model_type)
  {
    draw_model(object, camera_view, frame->graphics_command_buffer);
  }
  else
  {
    draw_buffer(object, camera_view, frame->graphics_command_buffer);
  }
}

auto Renderer::end() WIND_NOEXCEPT -> void
{
  auto* frame = &m_frame_context[m_current_frame];

  // end a dynamic render pass instance we already did required draw calls in draw() function
  // in begin() we did begin_rendering which says that we can record draw calls into cmd buffer after that
  // we did it in draw() now we have to end the instance no more draw cmd records
  frame->graphics_command_buffer.endRendering();

  // transition image from color_attachment_optimal to present_src_khr so presentation engine can
  // display it to monitor previous layout was color_attachment which was suitable for rendering now
  // we want a layout which will be suitable for presentation engine
  sync::transition_image(frame->graphics_command_buffer, m_swapchain_context.images[m_current_image],
                         vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR);

  // vk::EndCmdBuffer
  // when we did begin the state of command buffer was in recording now after end the state will transition to executable state
  // the specs says: The command buffer must have been in recording state, and if successful, is moved to executable state
  // so we can't call vk::EndCmdBuffer if we first did not called vk::CmdBegin
  // a command buffer in executable state can be submitted for execution, reset, freed or record another command buffer
  if(!frame->end())
    return;

  vk::CommandBufferSubmitInfo cmd_buffer_submit_info{};
  cmd_buffer_submit_info.commandBuffer = frame->graphics_command_buffer;

  // this is wait semaphore remember in acquire_image call we set this image_available semaphore
  // so once acquire calls return the semaphore state will transition to signaled
  // stageMask = eColorAttachmentOutput so the idea is we can do previous pipeline operations
  // like vertex shader, primitive assembly, rasterization do those but before doing final
  // color attachment wait for swapchain image the image_available semaphore must needs to be signaled
  // before processing colorAttachmentOutput stage but wait for presentation engine to give image to render into
  vk::SemaphoreSubmitInfo wait_semaphore_info{};
  wait_semaphore_info.semaphore = *frame->image_available;
  wait_semaphore_info.stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;

  // this is opposite this is for presentation engine it needs to know when GPU has finished rendering current image
  // so it can present to monitor safely prevents thing like presentation engine display image while gpu drawing into weird :(
  // it does not wait for specific operations it just signaled setting semaphore to signaled
  vk::SemaphoreSubmitInfo render_finished_semaphore_info{};
  render_finished_semaphore_info.semaphore = *frame->render_finished;
  render_finished_semaphore_info.stageMask = vk::PipelineStageFlagBits2::eAllGraphics;

  vk::SubmitInfo2 submit_info{};

  submit_info.commandBufferInfoCount = 1;
  submit_info.pCommandBufferInfos    = &cmd_buffer_submit_info;

  // wait semaphore GPU waits for that to be signaled
  submit_info.waitSemaphoreInfoCount = 1;
  submit_info.pWaitSemaphoreInfos    = &wait_semaphore_info;

  // GPU does not it just signal when it hits stageMask
  submit_info.signalSemaphoreInfoCount = 1;
  submit_info.pSignalSemaphoreInfos    = &render_finished_semaphore_info;

  // async call to submit vulkan will process things internally and send to GPU note: the specs says that
  // command buffer from executable state (vk::CmdEnd) can be transition to pending by queue submission command
  // in pending state the application "must not modify command buffer" (never forget about it) as it may be executing on device
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
  // wait for render_finished to be signaled in submit info it will happen when gpu reaches eAllGraphics stage
  // after than the current image has been rendered and presentation engine can finally sent to OS
  present_info.pWaitSemaphores = &*frame->render_finished;
  present_info.pImageIndices   = &m_current_image;

  VkPresentInfoKHR present_info_c = static_cast<VkPresentInfoKHR>(present_info);
  VkResult         present_result =
      m_context->gpu_device.device.getDispatcher()->vkQueuePresentKHR(*m_context->gpu_device.presentation_queue, &present_info_c);
  auto result = static_cast<vk::Result>(present_result);

  if(result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR && result != vk::Result::eErrorOutOfDateKHR)
  {
    spdlog::info("Failed to present");
  }

  m_current_frame = (m_current_frame + 1) % MAX_FRAME_IN_FLIGHT;
}

}  // namespace wind::vulkan
