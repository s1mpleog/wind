#include "Pipeline.hpp"
#include "./Error.hpp"
#include "spdlog/spdlog.h"
#include "./Types.hpp"
#include "utils/ExpectedUtil.hpp"
#include "vulkan/graphics/PipelineConfig.hpp"
#include "vulkan/Types.hpp"
#include "vulkan/vulkan.hpp"
#include <array>
#include <vulkan/vulkan_raii.hpp>

namespace wind::vulkan::graphics {
WIND_NODISCARD auto create(const vk::raii::Device& device, GraphicsConfig cfg) WIND_NOEXCEPT -> WindResult<GraphicsPipeline>
{
  vk::GraphicsPipelineCreateInfo gp_create_info{};

  for(const auto& shader : cfg.shader)
  {
    spdlog::info("config shader module = {}", reinterpret_cast<uintptr_t>(shader.module));
  }

  auto                                   pipeline_create_info = to_vk(cfg);
  vk::PipelineVertexInputStateCreateInfo vertex_input{};
  vertex_input.vertexAttributeDescriptionCount = static_cast<u32>(pipeline_create_info.attributes.size());
  vertex_input.pVertexAttributeDescriptions    = pipeline_create_info.attributes.data();
  vertex_input.vertexBindingDescriptionCount   = static_cast<u32>(pipeline_create_info.bindings.size());
  vertex_input.pVertexBindingDescriptions      = pipeline_create_info.bindings.data();

  vk::PipelineColorBlendStateCreateInfo color_blend{};
  color_blend.attachmentCount = 1;
  color_blend.pAttachments    = &pipeline_create_info.color_blend_attachment;

  std::array                         dynamic_states{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
  vk::PipelineDynamicStateCreateInfo dynamic_state_info{};
  dynamic_state_info.dynamicStateCount = dynamic_states.size();
  dynamic_state_info.pDynamicStates    = dynamic_states.data();

  vk::PipelineViewportStateCreateInfo viewport_state{};
  viewport_state.viewportCount = 1;
  viewport_state.scissorCount  = 1;

  vk::Format color_attachment_format = to_vk(cfg.color_format);

  vk::PipelineRenderingCreateInfo rendering_info{};
  rendering_info.colorAttachmentCount    = 1;
  rendering_info.pColorAttachmentFormats = &color_attachment_format;
  rendering_info.depthAttachmentFormat   = to_vk(cfg.depth_format);
  rendering_info.stencilAttachmentFormat = vk::Format::eUndefined;


  std::vector<vk::PushConstantRange> push_constant_ranges;
  push_constant_ranges.reserve(cfg.push_constants.size());

  for(auto& push_constant : cfg.push_constants)
  {
    push_constant_ranges.push_back(to_vk(push_constant));
  }

  vk::PipelineLayoutCreateInfo layout_info{};
  layout_info.pushConstantRangeCount = static_cast<u32>(push_constant_ranges.size());
  layout_info.pPushConstantRanges    = push_constant_ranges.data();

  if(cfg.descriptor_set_layout)
  {
    // since we are using bindless descriptor set we only need one layout
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts    = &*cfg.descriptor_set_layout;
  }

  auto layout = WIND_TRY(device.createPipelineLayout(layout_info), ErrorCode::FailedToCreatePipelineLayout);

  gp_create_info.pNext               = &rendering_info;
  gp_create_info.stageCount          = pipeline_create_info.shader_stages.size();
  gp_create_info.pStages             = pipeline_create_info.shader_stages.data();
  gp_create_info.pInputAssemblyState = &pipeline_create_info.input_assembly;
  gp_create_info.pViewportState      = &viewport_state;
  gp_create_info.pVertexInputState   = &vertex_input;
  gp_create_info.pTessellationState  = nullptr;
  gp_create_info.pRasterizationState = &pipeline_create_info.rasterization;
  gp_create_info.pMultisampleState   = &pipeline_create_info.multisample;
  gp_create_info.pDepthStencilState  = &pipeline_create_info.depth_stencil;
  gp_create_info.pColorBlendState    = &color_blend;
  gp_create_info.pDynamicState       = &dynamic_state_info;
  gp_create_info.renderPass          = nullptr;
  gp_create_info.layout              = layout;

  auto graphics_pipeline = WIND_TRY(device.createGraphicsPipeline(nullptr, gp_create_info), ErrorCode::FailedToCreatePipeline);

  const auto& a = pipeline_create_info.color_blend_attachment;

  spdlog::info("blend enable: {}", bool(a.blendEnable));
  spdlog::info("color write mask: {}", vk::to_string(a.colorWriteMask));
  spdlog::info("src color: {}", vk::to_string(a.srcColorBlendFactor));
  spdlog::info("dst color: {}", vk::to_string(a.dstColorBlendFactor));
  spdlog::info("color op: {}", vk::to_string(a.colorBlendOp));
  spdlog::info("src alpha: {}", vk::to_string(a.srcAlphaBlendFactor));
  spdlog::info("dst alpha: {}", vk::to_string(a.dstAlphaBlendFactor));
  spdlog::info("alpha op: {}", vk::to_string(a.alphaBlendOp));

  return GraphicsPipeline{.pipeline_layout = std::move(layout), .graphics_pipeline = std::move(graphics_pipeline)};
}

}  // namespace wind::vulkan::graphics
