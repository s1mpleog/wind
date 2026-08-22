#include "pipeline.hpp"
#include "error.hpp"
#include "spdlog/spdlog.h"
#include "utils/expected_util.hpp"
#include "vulkan/vulkan.hpp"
#include <array>

namespace wind::vulkan::graphics {
WIND_NODISCARD auto create(const vk::raii::Device& device) WIND_NOEXCEPT -> WindResult<vk::raii::Pipeline>
{
  vk::GraphicsPipelineCreateInfo gp_create_info{};

  // obviously in config
  vk::PipelineShaderStageCreateInfo shader_create_info{};
  shader_create_info.stage = vk::ShaderStageFlagBits::eVertex;

  vk::VertexInputAttributeDescription attrs{};
  attrs.format = vk::Format::eA1B5G5R5UnormPack16;

  // this will go in config
  vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
  vertex_input_info.vertexAttributeDescriptionCount = 0;
  vertex_input_info.pVertexAttributeDescriptions    = nullptr;
  vertex_input_info.vertexBindingDescriptionCount   = 0;
  vertex_input_info.pVertexBindingDescriptions      = nullptr;

  vk::PipelineInputAssemblyStateCreateInfo input_assembly_info{};
  input_assembly_info.topology               = vk::PrimitiveTopology::eTriangleList;
  input_assembly_info.primitiveRestartEnable = vk::False;

  // this will go in config
  vk::PipelineRasterizationStateCreateInfo rasterization_state_info{};
  rasterization_state_info.cullMode                = vk::CullModeFlagBits::eBack;
  rasterization_state_info.polygonMode             = vk::PolygonMode::eFill;
  rasterization_state_info.rasterizerDiscardEnable = vk::False;
  rasterization_state_info.frontFace               = vk::FrontFace::eCounterClockwise;
  rasterization_state_info.depthBiasEnable         = vk::False;
  rasterization_state_info.depthClampEnable        = vk::False;

  // maybe this will go in config
  vk::PipelineMultisampleStateCreateInfo multi_sample_info{};
  multi_sample_info.sampleShadingEnable  = vk::False;
  multi_sample_info.rasterizationSamples = vk::SampleCountFlagBits::e1;

  // this will go in config
  vk::PipelineDepthStencilStateCreateInfo depth_stencil_info{};
  depth_stencil_info.depthTestEnable       = vk::False;
  depth_stencil_info.depthWriteEnable      = vk::False;
  depth_stencil_info.depthCompareOp        = vk::CompareOp::eLess;
  depth_stencil_info.depthBoundsTestEnable = vk::False;
  depth_stencil_info.stencilTestEnable     = vk::False;

  // this will go in config
  vk::PipelineColorBlendStateCreateInfo color_blend_info{};
  color_blend_info.attachmentCount = 0;
  color_blend_info.logicOpEnable   = vk::False;

  // this will go in config
  std::array                         dynamic_states{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
  vk::PipelineDynamicStateCreateInfo dynamic_state_info{};
  dynamic_state_info.dynamicStateCount = dynamic_states.size();
  dynamic_state_info.pDynamicStates    = dynamic_states.data();

  // from config
  vk::PipelineLayoutCreateInfo layout_info{};
  layout_info.setLayoutCount = 0;

  auto layout = WIND_TRY(device.createPipelineLayout(layout_info), ErrorCode::FailedToCreatePipelineLayout);

  // dynamic rendering

  // from config
  vk::Format color_attachment_format = vk::Format::eB8G8R8A8Srgb;

  vk::PipelineRenderingCreateInfo rendering_info{};
  rendering_info.colorAttachmentCount    = 1;
  rendering_info.pColorAttachmentFormats = &color_attachment_format;
  rendering_info.depthAttachmentFormat   = vk::Format::eUndefined;
  rendering_info.stencilAttachmentFormat = vk::Format::eUndefined;

  gp_create_info.pNext      = &rendering_info;
  gp_create_info.stageCount = 0;
  // gp_create_info.pStages             = &shader_create_info;
  gp_create_info.pInputAssemblyState = &input_assembly_info;
  gp_create_info.pVertexInputState   = &vertex_input_info;
  gp_create_info.pTessellationState  = nullptr;
  gp_create_info.pRasterizationState = &rasterization_state_info;
  gp_create_info.pMultisampleState   = &multi_sample_info;
  gp_create_info.pDepthStencilState  = &depth_stencil_info;
  gp_create_info.pColorBlendState    = &color_blend_info;
  gp_create_info.pDynamicState       = &dynamic_state_info;
  gp_create_info.renderPass          = nullptr;
  gp_create_info.layout              = layout;

  auto graphics_pipeline = WIND_TRY(device.createGraphicsPipeline(nullptr, gp_create_info), ErrorCode::FailedToCreatePipeline);

  spdlog::info("graphics pipeline created success: {}", (void*)*graphics_pipeline);

  return nullptr;
}

}  // namespace wind::vulkan::graphics
