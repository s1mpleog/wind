#include "pipeline.hpp"
#include "error.hpp"
#include "spdlog/spdlog.h"
#include "utils/expected_util.hpp"
#include "vulkan/graphics/pipeline_config.hpp"
#include "vulkan/types.hpp"
#include "vulkan/vulkan.hpp"
#include <array>

namespace wind::vulkan::graphics {
WIND_NODISCARD auto create(const vk::raii::Device& device, GraphicsConfig cfg) WIND_NOEXCEPT -> WindResult<PipelineInfo>
{
  vk::GraphicsPipelineCreateInfo gp_create_info{};

  auto pipeline_create_info = to_vk(cfg);

  vk::PipelineShaderStageCreateInfo        shader_info         = pipeline_create_info.shader_stages;
  vk::PipelineRasterizationStateCreateInfo rasterization_info  = pipeline_create_info.rasterization;
  vk::PipelineVertexInputStateCreateInfo   vertex_info         = pipeline_create_info.vertex_input;
  vk::PipelineInputAssemblyStateCreateInfo input_assembly_info = pipeline_create_info.input_assembly;
  vk::PipelineMultisampleStateCreateInfo   multi_sample_info   = pipeline_create_info.multisample;
  vk::PipelineColorBlendStateCreateInfo    color_blend_info    = pipeline_create_info.color_blend;
  vk::PipelineDepthStencilStateCreateInfo  depth_stencil_info  = pipeline_create_info.depth_stencil;

  std::array                         dynamic_states{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
  vk::PipelineDynamicStateCreateInfo dynamic_state_info{};
  dynamic_state_info.dynamicStateCount = dynamic_states.size();
  dynamic_state_info.pDynamicStates    = dynamic_states.data();

  vk::Format color_attachment_format = to_vk(cfg.color_format);

  vk::PipelineRenderingCreateInfo rendering_info{};
  rendering_info.colorAttachmentCount    = 1;
  rendering_info.pColorAttachmentFormats = &color_attachment_format;
  rendering_info.depthAttachmentFormat   = to_vk(cfg.depth_format);
  rendering_info.stencilAttachmentFormat = vk::Format::eUndefined;

  vk::PipelineLayoutCreateInfo layout_info{};
  layout_info.setLayoutCount = 0;

  auto layout = WIND_TRY(device.createPipelineLayout(layout_info), ErrorCode::FailedToCreatePipelineLayout);

  gp_create_info.pNext               = &rendering_info;
  gp_create_info.stageCount          = cfg.shader.size();
  gp_create_info.pStages             = &shader_info;
  gp_create_info.pInputAssemblyState = &input_assembly_info;
  gp_create_info.pVertexInputState   = &vertex_info;
  gp_create_info.pTessellationState  = nullptr;
  gp_create_info.pRasterizationState = &rasterization_info;
  gp_create_info.pMultisampleState   = &multi_sample_info;
  gp_create_info.pDepthStencilState  = &depth_stencil_info;
  gp_create_info.pColorBlendState    = &color_blend_info;
  gp_create_info.pDynamicState       = &dynamic_state_info;
  gp_create_info.renderPass          = nullptr;
  gp_create_info.layout              = layout;

  auto graphics_pipeline = WIND_TRY(device.createGraphicsPipeline(nullptr, gp_create_info), ErrorCode::FailedToCreatePipeline);

  spdlog::info("graphics pipeline created success: {}", (void*)*graphics_pipeline);


  return PipelineInfo{.pipeline_layout = std::move(layout), .graphics_pipeline = std::move(graphics_pipeline)};
}

}  // namespace wind::vulkan::graphics
