#pragma once

#include "config.hpp"
#include "vulkan/core/configuration.hpp"
#include "vulkan/graphics/pipeline_config.hpp"
#include "vulkan/vulkan_core.h"
#include <span>
#include <vector>

namespace wind::vulkan {

constexpr auto to_vk(VulkanVersion value) WIND_NOEXCEPT -> uint32_t
{
  switch(value)
  {
    case VulkanVersion::VERSION13:
      return vk::ApiVersion13;

    case VulkanVersion::VERSION14:
      return vk::ApiVersion14;
  }

  std::unreachable();
}

constexpr auto to_vk(DebugMessageSeverity value) WIND_NOEXCEPT -> vk::DebugUtilsMessageSeverityFlagBitsEXT
{
  return static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(static_cast<u32>(value));
}

constexpr auto to_vk(DebugMessageType value) WIND_NOEXCEPT -> vk::DebugUtilsMessageTypeFlagsEXT
{
  return static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(static_cast<u32>(value));
}

auto to_vk(PrimitiveTopology topology) WIND_NOEXCEPT -> vk::PrimitiveTopology;
auto to_vk(CullMode cull_mode) WIND_NOEXCEPT -> vk::CullModeFlagBits;
auto to_vk(PolygonMode polygon_mode) WIND_NOEXCEPT -> vk::PolygonMode;
auto to_vk(FrontFace front_face) WIND_NOEXCEPT -> vk::FrontFace;
auto to_vk(Format format) WIND_NOEXCEPT -> vk::Format;
auto to_vk(VertexFormat format) WIND_NOEXCEPT -> vk::Format;
auto to_vk(VertexInputRate input_rate) WIND_NOEXCEPT -> vk::VertexInputRate;
auto to_vk(CompareOp compare_op) WIND_NOEXCEPT -> vk::CompareOp;
auto to_vk(ShaderStage stage) WIND_NOEXCEPT -> vk::ShaderStageFlagBits;
auto to_vk(const VertexAttribute& attribute) WIND_NOEXCEPT -> vk::VertexInputAttributeDescription;
auto to_vk(const VertexBinding& binding) WIND_NOEXCEPT -> vk::VertexInputBindingDescription;
auto to_vk(const VertexInputState& vertex_input) WIND_NOEXCEPT
    -> std::pair<std::vector<vk::VertexInputAttributeDescription>, std::vector<vk::VertexInputBindingDescription>>;
auto to_vk(const RasterizationState& rasterization) WIND_NOEXCEPT -> vk::PipelineRasterizationStateCreateInfo;
auto to_vk(const InputAssemblyState& input_assembly) WIND_NOEXCEPT -> vk::PipelineInputAssemblyStateCreateInfo;
auto to_vk(const DepthStencilState& depth_stencil) WIND_NOEXCEPT -> vk::PipelineDepthStencilStateCreateInfo;
auto to_vk(BlendFactor blend_factor) WIND_NOEXCEPT -> vk::BlendFactor;
auto to_vk(BlendOp blend_op) WIND_NOEXCEPT -> vk::BlendOp;
auto to_vk(const ColorBlendState& color_blend) WIND_NOEXCEPT -> vk::PipelineColorBlendAttachmentState;

// Implementation

WIND_INLINE auto to_vk(PrimitiveTopology topology) WIND_NOEXCEPT -> vk::PrimitiveTopology
{
  switch(topology)
  {
    case PrimitiveTopology::PointList:
      return vk::PrimitiveTopology::ePointList;
    case PrimitiveTopology::LineList:
      return vk::PrimitiveTopology::eLineList;
    case PrimitiveTopology::LineStrip:
      return vk::PrimitiveTopology::eLineStrip;
    case PrimitiveTopology::TriangleList:
      return vk::PrimitiveTopology::eTriangleList;
    case PrimitiveTopology::TriangleStrip:
      return vk::PrimitiveTopology::eTriangleStrip;
    case PrimitiveTopology::TriangleFan:
      return vk::PrimitiveTopology::eTriangleFan;
    default:
      return vk::PrimitiveTopology::eTriangleList;
  }
}

WIND_INLINE auto to_vk(CullMode cull_mode) WIND_NOEXCEPT -> vk::CullModeFlagBits
{
  switch(cull_mode)
  {
    case CullMode::Back:
      return vk::CullModeFlagBits::eBack;
    case CullMode::FontAndBack:
      return vk::CullModeFlagBits::eFrontAndBack;
    case CullMode::None:
      return vk::CullModeFlagBits::eNone;
    default:
      return vk::CullModeFlagBits::eNone;
  }
}

WIND_INLINE auto to_vk(PolygonMode polygon_mode) WIND_NOEXCEPT -> vk::PolygonMode
{
  switch(polygon_mode)
  {
    case PolygonMode::Fill:
      return vk::PolygonMode::eFill;
    case PolygonMode::Line:
      return vk::PolygonMode::eLine;
    case PolygonMode::Point:
      return vk::PolygonMode::ePoint;
    default:
      return vk::PolygonMode::eFill;
  }
}

WIND_INLINE auto to_vk(FrontFace front_face) WIND_NOEXCEPT -> vk::FrontFace
{
  switch(front_face)
  {
    case FrontFace::CounterClockwise:
      return vk::FrontFace::eCounterClockwise;
    case FrontFace::ClockWise:
      return vk::FrontFace::eClockwise;
    default:
      return vk::FrontFace::eClockwise;
  }
}

WIND_INLINE auto to_vk(Format format) WIND_NOEXCEPT -> vk::Format
{
  switch(format)
  {
    case Format::Undefined:
      return vk::Format::eUndefined;

    // 8-bit formats
    case Format::R8_UNORM:
      return vk::Format::eR8Unorm;
    case Format::R8_SNORM:
      return vk::Format::eR8Snorm;
    case Format::R8_UINT:
      return vk::Format::eR8Uint;
    case Format::R8_SINT:
      return vk::Format::eR8Sint;
    case Format::R8_SRGB:
      return vk::Format::eR8Srgb;

    case Format::RG8_UNORM:
      return vk::Format::eR8G8Unorm;
    case Format::RG8_SNORM:
      return vk::Format::eR8G8Snorm;
    case Format::RG8_UINT:
      return vk::Format::eR8G8Uint;
    case Format::RG8_SINT:
      return vk::Format::eR8G8Sint;
    case Format::RG8_SRGB:
      return vk::Format::eR8G8Srgb;

    case Format::RGBA8_UNORM:
      return vk::Format::eR8G8B8A8Unorm;
    case Format::RGBA8_SNORM:
      return vk::Format::eR8G8B8A8Snorm;
    case Format::RGBA8_UINT:
      return vk::Format::eR8G8B8A8Uint;
    case Format::RGBA8_SINT:
      return vk::Format::eR8G8B8A8Sint;
    case Format::RGBA8_SRGB:
      return vk::Format::eR8G8B8A8Srgb;

    case Format::BGRA8_UNORM:
      return vk::Format::eB8G8R8A8Unorm;
    case Format::BGRA8_SRGB:
      return vk::Format::eB8G8R8A8Srgb;

    // Packed formats
    case Format::RGB565_UNORM_PACK16:
      return vk::Format::eR5G6B5UnormPack16;
    case Format::RGBA4_UNORM_PACK16:
      return vk::Format::eR4G4B4A4UnormPack16;
    case Format::RGB5A1_UNORM_PACK16:
      return vk::Format::eR5G5B5A1UnormPack16;
    case Format::A2B10G10R10_UNORM_PACK32:
      return vk::Format::eA2B10G10R10UnormPack32;
    case Format::A2B10G10R10_UINT_PACK32:
      return vk::Format::eA2B10G10R10UintPack32;
    case Format::A2R10G10B10_UNORM_PACK32:
      return vk::Format::eA2R10G10B10UnormPack32;
    case Format::B10G11R11_UFLOAT_PACK32:
      return vk::Format::eB10G11R11UfloatPack32;
    case Format::E5B9G9R9_UFLOAT_PACK32:
      return vk::Format::eE5B9G9R9UfloatPack32;

    // 16-bit formats
    case Format::R16_UNORM:
      return vk::Format::eR16Unorm;
    case Format::R16_SNORM:
      return vk::Format::eR16Snorm;
    case Format::R16_UINT:
      return vk::Format::eR16Uint;
    case Format::R16_SINT:
      return vk::Format::eR16Sint;
    case Format::R16_FLOAT:
      return vk::Format::eR16Sfloat;

    case Format::RG16_UNORM:
      return vk::Format::eR16G16Unorm;
    case Format::RG16_SNORM:
      return vk::Format::eR16G16Snorm;
    case Format::RG16_UINT:
      return vk::Format::eR16G16Uint;
    case Format::RG16_SINT:
      return vk::Format::eR16G16Sint;
    case Format::RG16_FLOAT:
      return vk::Format::eR16G16Sfloat;

    case Format::RGBA16_UNORM:
      return vk::Format::eR16G16B16A16Unorm;
    case Format::RGBA16_SNORM:
      return vk::Format::eR16G16B16A16Snorm;
    case Format::RGBA16_UINT:
      return vk::Format::eR16G16B16A16Uint;
    case Format::RGBA16_SINT:
      return vk::Format::eR16G16B16A16Sint;
    case Format::RGBA16_FLOAT:
      return vk::Format::eR16G16B16A16Sfloat;

    // 32-bit formats
    case Format::R32_UINT:
      return vk::Format::eR32Uint;
    case Format::R32_SINT:
      return vk::Format::eR32Sint;
    case Format::R32_FLOAT:
      return vk::Format::eR32Sfloat;

    case Format::RG32_UINT:
      return vk::Format::eR32G32Uint;
    case Format::RG32_SINT:
      return vk::Format::eR32G32Sint;
    case Format::RG32_FLOAT:
      return vk::Format::eR32G32Sfloat;

    case Format::RGB32_UINT:
      return vk::Format::eR32G32B32Uint;
    case Format::RGB32_SINT:
      return vk::Format::eR32G32B32Sint;
    case Format::RGB32_FLOAT:
      return vk::Format::eR32G32B32Sfloat;

    case Format::RGBA32_UINT:
      return vk::Format::eR32G32B32A32Uint;
    case Format::RGBA32_SINT:
      return vk::Format::eR32G32B32A32Sint;
    case Format::RGBA32_FLOAT:
      return vk::Format::eR32G32B32A32Sfloat;

    // Depth/Stencil formats
    case Format::D16_UNORM:
      return vk::Format::eD16Unorm;
    case Format::X8_D24_UNORM_PACK32:
      return vk::Format::eX8D24UnormPack32;
    case Format::D32_FLOAT:
      return vk::Format::eD32Sfloat;
    case Format::S8_UINT:
      return vk::Format::eS8Uint;
    case Format::D16_UNORM_S8_UINT:
      return vk::Format::eD16UnormS8Uint;
    case Format::D24_UNORM_S8_UINT:
      return vk::Format::eD24UnormS8Uint;
    case Format::D32_FLOAT_S8_UINT:
      return vk::Format::eD32SfloatS8Uint;

    // BC compressed formats
    case Format::BC1_RGB_UNORM_BLOCK:
      return vk::Format::eBc1RgbUnormBlock;
    case Format::BC1_RGB_SRGB_BLOCK:
      return vk::Format::eBc1RgbSrgbBlock;
    case Format::BC1_RGBA_UNORM_BLOCK:
      return vk::Format::eBc1RgbaUnormBlock;
    case Format::BC1_RGBA_SRGB_BLOCK:
      return vk::Format::eBc1RgbaSrgbBlock;
    case Format::BC2_UNORM_BLOCK:
      return vk::Format::eBc2UnormBlock;
    case Format::BC2_SRGB_BLOCK:
      return vk::Format::eBc2SrgbBlock;
    case Format::BC3_UNORM_BLOCK:
      return vk::Format::eBc3UnormBlock;
    case Format::BC3_SRGB_BLOCK:
      return vk::Format::eBc3SrgbBlock;
    case Format::BC4_UNORM_BLOCK:
      return vk::Format::eBc4UnormBlock;
    case Format::BC4_SNORM_BLOCK:
      return vk::Format::eBc4SnormBlock;
    case Format::BC5_UNORM_BLOCK:
      return vk::Format::eBc5UnormBlock;
    case Format::BC5_SNORM_BLOCK:
      return vk::Format::eBc5SnormBlock;
    case Format::BC6H_UFLOAT_BLOCK:
      return vk::Format::eBc6HUfloatBlock;
    case Format::BC6H_SFLOAT_BLOCK:
      return vk::Format::eBc6HSfloatBlock;
    case Format::BC7_UNORM_BLOCK:
      return vk::Format::eBc7UnormBlock;
    case Format::BC7_SRGB_BLOCK:
      return vk::Format::eBc7SrgbBlock;

    default:
      return vk::Format::eUndefined;
  }
}

WIND_INLINE auto to_vk(VertexFormat format) WIND_NOEXCEPT -> vk::Format
{
  switch(format)
  {
    case VertexFormat::Float:
      return vk::Format::eR32Sfloat;
    case VertexFormat::Float2:
      return vk::Format::eR32G32Sfloat;
    case VertexFormat::Float3:
      return vk::Format::eR32G32B32Sfloat;
    case VertexFormat::Float4:
      return vk::Format::eR32G32B32A32Sfloat;

    case VertexFormat::Int:
      return vk::Format::eR32Sint;
    case VertexFormat::Int2:
      return vk::Format::eR32G32Sint;
    case VertexFormat::Int3:
      return vk::Format::eR32G32B32Sint;
    case VertexFormat::Int4:
      return vk::Format::eR32G32B32A32Sint;

    case VertexFormat::UInt:
      return vk::Format::eR32Uint;
    case VertexFormat::UInt2:
      return vk::Format::eR32G32Uint;
    case VertexFormat::UInt3:
      return vk::Format::eR32G32B32Uint;
    case VertexFormat::UInt4:
      return vk::Format::eR32G32B32A32Uint;

    default:
      return vk::Format::eR32Sfloat;
  }
}

WIND_INLINE auto to_vk(VertexInputRate input_rate) WIND_NOEXCEPT -> vk::VertexInputRate
{
  switch(input_rate)
  {
    case VertexInputRate::Vertex:
      return vk::VertexInputRate::eVertex;
    case VertexInputRate::Instance:
      return vk::VertexInputRate::eInstance;
    default:
      return vk::VertexInputRate::eVertex;
  }
}

WIND_INLINE auto to_vk(const VertexAttribute& attribute) WIND_NOEXCEPT -> vk::VertexInputAttributeDescription
{
  return vk::VertexInputAttributeDescription{attribute.location, attribute.binding, to_vk(attribute.format), attribute.offset};
}

WIND_INLINE auto to_vk(std::span<const VertexAttribute> attributes) WIND_NOEXCEPT -> std::vector<vk::VertexInputAttributeDescription>
{
  std::vector<vk::VertexInputAttributeDescription> vertex_attributes;
  vertex_attributes.reserve(attributes.size());

  for(const auto& attribute : attributes)
  {
    vertex_attributes.emplace_back(attribute.location, attribute.binding, to_vk(attribute.format), attribute.offset);
  }

  return vertex_attributes;
}

WIND_INLINE auto to_vk(const VertexBinding& binding) WIND_NOEXCEPT -> vk::VertexInputBindingDescription
{
  return vk::VertexInputBindingDescription{binding.binding, binding.stride, to_vk(binding.input_rate)};
}

WIND_INLINE auto to_vk(std::span<const VertexBinding> bindings) WIND_NOEXCEPT -> std::vector<vk::VertexInputBindingDescription>
{
  std::vector<vk::VertexInputBindingDescription> vertex_bindings;
  vertex_bindings.reserve(bindings.size());

  for(const auto& binding : bindings)
  {
    vertex_bindings.emplace_back(binding.binding, binding.stride, to_vk(binding.input_rate));
  }

  return vertex_bindings;
}

WIND_INLINE auto to_vk(const VertexInputState& vertex_input) WIND_NOEXCEPT
    -> std::pair<std::vector<vk::VertexInputAttributeDescription>, std::vector<vk::VertexInputBindingDescription>>
{
  std::vector<vk::VertexInputAttributeDescription> attributes;
  std::vector<vk::VertexInputBindingDescription>   bindings;

  attributes.reserve(vertex_input.attributes.size());
  bindings.reserve(vertex_input.bindings.size());

  for(const auto& attr : vertex_input.attributes)
    attributes.push_back(to_vk(attr));

  for(const auto& bind : vertex_input.bindings)
    bindings.push_back(to_vk(bind));

  return {std::move(attributes), std::move(bindings)};
}

WIND_INLINE auto to_vk(CompareOp compare_op) WIND_NOEXCEPT -> vk::CompareOp
{
  switch(compare_op)
  {
    case CompareOp::Less:
      return vk::CompareOp::eLess;
    case CompareOp::Always:
      return vk::CompareOp::eAlways;
    case CompareOp::Never:
      return vk::CompareOp::eNever;
    case CompareOp::Greater:
      return vk::CompareOp::eGreater;
    default:
      return vk::CompareOp::eLess;
  }
}

WIND_INLINE auto to_vk(ShaderStage stage) WIND_NOEXCEPT -> vk::ShaderStageFlagBits
{
  switch(stage)
  {
    case ShaderStage::Vertex:
      return vk::ShaderStageFlagBits::eVertex;
    case ShaderStage::Fragment:
      return vk::ShaderStageFlagBits::eFragment;
    case ShaderStage::Compute:
      return vk::ShaderStageFlagBits::eCompute;
    default:
      return vk::ShaderStageFlagBits::eVertex;
  }
}

WIND_INLINE auto to_vk(const RasterizationState& rasterization) WIND_NOEXCEPT -> vk::PipelineRasterizationStateCreateInfo
{
  vk::PipelineRasterizationStateCreateInfo info = {};
  info.depthClampEnable                         = static_cast<vk::Bool32>(rasterization.depth_clamp);
  info.rasterizerDiscardEnable                  = static_cast<vk::Bool32>(rasterization.discard);
  info.polygonMode                              = to_vk(rasterization.polygon_mode);
  info.cullMode                                 = to_vk(rasterization.cull_mode);
  info.frontFace                                = to_vk(rasterization.front_face);
  info.lineWidth                                = 1.0F;

  return info;
}

WIND_INLINE auto to_vk(const InputAssemblyState& input_assembly) WIND_NOEXCEPT -> vk::PipelineInputAssemblyStateCreateInfo
{
  vk::PipelineInputAssemblyStateCreateInfo create_info{};
  create_info.topology               = to_vk(input_assembly.topology);
  create_info.primitiveRestartEnable = vk::False;

  return create_info;
}

WIND_INLINE auto to_vk(const DepthStencilState& depth_stencil) WIND_NOEXCEPT -> vk::PipelineDepthStencilStateCreateInfo
{
  vk::PipelineDepthStencilStateCreateInfo info{};

  info.depthTestEnable   = static_cast<vk::Bool32>(depth_stencil.depth_test);
  info.depthWriteEnable  = static_cast<vk::Bool32>(depth_stencil.depth_write);
  info.depthCompareOp    = to_vk(depth_stencil.depth_compare);
  info.stencilTestEnable = static_cast<vk::Bool32>(depth_stencil.stencil_test);

  // Default stencil ops if not configured
  info.front = vk::StencilOpState{vk::StencilOp::eKeep,
                                  vk::StencilOp::eKeep,
                                  vk::StencilOp::eKeep,
                                  vk::CompareOp::eAlways,
                                  0xFFFFFFFF,
                                  0xFFFFFFFF,
                                  0};

  info.back = info.front;

  return info;
}

WIND_INLINE auto to_vk(BlendFactor blend_factor) WIND_NOEXCEPT -> vk::BlendFactor
{
  switch(blend_factor)
  {
    case BlendFactor::Zero:
      return vk::BlendFactor::eZero;
    case BlendFactor::One:
      return vk::BlendFactor::eOne;
    case BlendFactor::SrcColor:
      return vk::BlendFactor::eSrcColor;
    case BlendFactor::OneMinusSrcColor:
      return vk::BlendFactor::eOneMinusSrcColor;
    case BlendFactor::DstColor:
      return vk::BlendFactor::eDstColor;
    case BlendFactor::OneMinusDstColor:
      return vk::BlendFactor::eOneMinusDstColor;
    case BlendFactor::SrcAlpha:
      return vk::BlendFactor::eSrcAlpha;
    case BlendFactor::OneMinusSrcAlpha:
      return vk::BlendFactor::eOneMinusSrcAlpha;
    case BlendFactor::DstAlpha:
      return vk::BlendFactor::eDstAlpha;
    case BlendFactor::OneMinusDstAlpha:
      return vk::BlendFactor::eOneMinusDstAlpha;
    case BlendFactor::ConstantColor:
      return vk::BlendFactor::eConstantColor;
    case BlendFactor::OneMinusConstantColor:
      return vk::BlendFactor::eOneMinusConstantColor;
    case BlendFactor::ConstantAlpha:
      return vk::BlendFactor::eConstantAlpha;
    case BlendFactor::OneMinusConstantAlpha:
      return vk::BlendFactor::eOneMinusConstantAlpha;
    case BlendFactor::SrcAlphaSaturate:
      return vk::BlendFactor::eSrcAlphaSaturate;
    case BlendFactor::Src1Color:
      return vk::BlendFactor::eSrc1Color;
    case BlendFactor::OneMinusSrc1Color:
      return vk::BlendFactor::eOneMinusSrc1Color;
    case BlendFactor::Src1Alpha:
      return vk::BlendFactor::eSrc1Alpha;
    case BlendFactor::OneMinusSrc1Alpha:
      return vk::BlendFactor::eOneMinusSrc1Alpha;
    default:
      return vk::BlendFactor::eZero;
  }
}

WIND_INLINE auto to_vk(BlendOp blend_op) WIND_NOEXCEPT -> vk::BlendOp
{
  switch(blend_op)
  {
    case BlendOp::Add:
      return vk::BlendOp::eAdd;
    case BlendOp::Subtract:
      return vk::BlendOp::eSubtract;
    case BlendOp::ReverseSubtract:
      return vk::BlendOp::eReverseSubtract;
    case BlendOp::Min:
      return vk::BlendOp::eMin;
    case BlendOp::Max:
      return vk::BlendOp::eMax;
    default:
      return vk::BlendOp::eAdd;
  }
}

WIND_INLINE auto to_vk(const ColorBlendState& color_blend) WIND_NOEXCEPT -> vk::PipelineColorBlendAttachmentState
{
  vk::PipelineColorBlendAttachmentState state{};

  if(!color_blend.enabled)
  {
    state.blendEnable    = vk::True;
    state.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                           | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    return state;
  }

  state.blendEnable         = vk::True;
  state.srcColorBlendFactor = to_vk(color_blend.src_color);
  state.dstColorBlendFactor = to_vk(color_blend.dst_color);
  state.colorBlendOp        = to_vk(color_blend.color_op);
  state.srcAlphaBlendFactor = to_vk(color_blend.src_alpha);
  state.dstAlphaBlendFactor = to_vk(color_blend.dst_alpha);
  state.alphaBlendOp        = to_vk(color_blend.alpha_op);
  state.colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                              | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

  return state;
}

// Helper function to convert entire GraphicsConfig
struct PipelineCreateInfo
{
  std::vector<vk::PipelineShaderStageCreateInfo>   shader_stages;
  std::vector<vk::VertexInputAttributeDescription> attributes;
  std::vector<vk::VertexInputBindingDescription>   bindings;
  vk::PipelineInputAssemblyStateCreateInfo         input_assembly;
  vk::PipelineRasterizationStateCreateInfo         rasterization;
  vk::PipelineDepthStencilStateCreateInfo          depth_stencil;
  vk::PipelineColorBlendAttachmentState            color_blend_attachment;
  vk::PipelineViewportStateCreateInfo              viewport_state;
  vk::PipelineMultisampleStateCreateInfo           multisample;
};

WIND_INLINE auto to_vk(const graphics::GraphicsConfig& config) WIND_NOEXCEPT -> PipelineCreateInfo
{
  PipelineCreateInfo result{};

  // Convert shader stages
  std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;
  shader_stages.reserve(config.shader.size());

  for(const auto& shader_info : config.shader)
  {
    vk::PipelineShaderStageCreateInfo info{};
    info.stage  = to_vk(shader_info.stage);
    info.module = *shader_info.module;
    info.pName  = shader_info.entry_point.data();

    shader_stages.push_back(info);
  }

  result.bindings   = to_vk(config.vertex_input_state.bindings);
  result.attributes = to_vk(config.vertex_input_state.attributes);

  // Convert other states
  auto input_assembly           = to_vk(config.input_assembly);
  auto rasterization            = to_vk(config.rasterization);
  auto depth_stencil            = to_vk(config.depth_stencil);
  result.color_blend_attachment = to_vk(config.color_blend);

  // Viewport state (default)
  vk::PipelineViewportStateCreateInfo viewport_state{};
  viewport_state.viewportCount = 1;
  viewport_state.scissorCount  = 1;

  // Multisample state (default)
  vk::PipelineMultisampleStateCreateInfo multisample{};
  multisample.rasterizationSamples = vk::SampleCountFlagBits::e1;

  result.shader_stages  = std::move(shader_stages);
  result.input_assembly = input_assembly;
  result.rasterization  = rasterization;
  result.depth_stencil  = depth_stencil;
  result.viewport_state = viewport_state;
  result.multisample    = multisample;

  return result;
}

};  // namespace wind::vulkan
