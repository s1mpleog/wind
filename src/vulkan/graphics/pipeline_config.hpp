#pragma once

#include "types.hpp"
#include <vector>

namespace wind::vulkan {

enum class PrimitiveTopology : uint8_t
{
  PointList,
  LineList,
  LineStrip,
  TriangleList,
  TriangleStrip,
  TriangleFan,
};

enum class CullMode : u8
{
  Back,
  FontAndBack,
  None,
};

enum class PolygonMode : u8
{
  Fill,
  Line,
  Point
};

enum class FrontFace : u8
{
  CounterClockwise,
  ClockWise
};

enum class Format : uint16_t
{
  Undefined,

  // --- 8-bit Unsigned / Signed / Int Formats ---
  R8_UNORM,
  R8_SNORM,
  R8_UINT,
  R8_SINT,
  R8_SRGB,

  RG8_UNORM,
  RG8_SNORM,
  RG8_UINT,
  RG8_SINT,
  RG8_SRGB,

  RGBA8_UNORM,
  RGBA8_SNORM,
  RGBA8_UINT,
  RGBA8_SINT,
  RGBA8_SRGB,

  BGRA8_UNORM,
  BGRA8_SRGB,

  // --- Packed 8/10/16-bit Normalized Formats ---
  RGB565_UNORM_PACK16,
  RGBA4_UNORM_PACK16,
  RGB5A1_UNORM_PACK16,
  A2B10G10R10_UNORM_PACK32,
  A2B10G10R10_UINT_PACK32,
  A2R10G10B10_UNORM_PACK32,
  B10G11R11_UFLOAT_PACK32,
  E5B9G9R9_UFLOAT_PACK32,

  // --- 16-bit Formats ---
  R16_UNORM,
  R16_SNORM,
  R16_UINT,
  R16_SINT,
  R16_FLOAT,

  RG16_UNORM,
  RG16_SNORM,
  RG16_UINT,
  RG16_SINT,
  RG16_FLOAT,

  RGBA16_UNORM,
  RGBA16_SNORM,
  RGBA16_UINT,
  RGBA16_SINT,
  RGBA16_FLOAT,

  // --- 32-bit Formats ---
  R32_UINT,
  R32_SINT,
  R32_FLOAT,

  RG32_UINT,
  RG32_SINT,
  RG32_FLOAT,

  RGB32_UINT,
  RGB32_SINT,
  RGB32_FLOAT,

  RGBA32_UINT,
  RGBA32_SINT,
  RGBA32_FLOAT,

  // --- Depth / Stencil Formats ---
  D16_UNORM,
  X8_D24_UNORM_PACK32,
  D32_FLOAT,
  S8_UINT,
  D16_UNORM_S8_UINT,
  D24_UNORM_S8_UINT,
  D32_FLOAT_S8_UINT,

  // --- BC Compressed Textures (Desktop / Core) ---
  BC1_RGB_UNORM_BLOCK,
  BC1_RGB_SRGB_BLOCK,
  BC1_RGBA_UNORM_BLOCK,
  BC1_RGBA_SRGB_BLOCK,
  BC2_UNORM_BLOCK,
  BC2_SRGB_BLOCK,
  BC3_UNORM_BLOCK,
  BC3_SRGB_BLOCK,
  BC4_UNORM_BLOCK,
  BC4_SNORM_BLOCK,
  BC5_UNORM_BLOCK,
  BC5_SNORM_BLOCK,
  BC6H_UFLOAT_BLOCK,
  BC6H_SFLOAT_BLOCK,
  BC7_UNORM_BLOCK,
  BC7_SRGB_BLOCK,

};

enum class VertexFormat : u8
{
  Float,
  Float2,
  Float3,
  Float4,

  Int,
  Int2,
  Int3,
  Int4,

  UInt,
  UInt2,
  UInt3,
  UInt4,
};

enum class VertexInputRate : u8
{
  Vertex,
  Instance,
};

struct VertexAttribute
{
  u32          location{};
  u32          binding{};
  VertexFormat format{};
  u32          offset{};
};

struct VertexBinding
{
  uint32_t        binding;
  uint32_t        stride;
  VertexInputRate input_rate;
};

struct VertexInputState
{
  std::vector<VertexAttribute> attributes;
  std::vector<VertexBinding>   bindings;
};

enum class CompareOp : u8
{
  Less,
  Always,
  Never,
  Greater
};

enum class ShaderStage : u8
{
  Vertex,
  Fragment,
  Compute
};

struct ShaderInfo
{
  ShaderStage                   stage{ShaderStage::Vertex};
  std::string_view              entry_point{"main"};
  const vk::raii::ShaderModule* module{nullptr};
};

struct RasterizationState
{
  CullMode    cull_mode{CullMode::Back};
  PolygonMode polygon_mode{PolygonMode::Fill};
  FrontFace   front_face{FrontFace::ClockWise};

  bool depth_clamp{false};
  bool discard{false};
};

struct InputAssemblyState
{
  PrimitiveTopology topology{PrimitiveTopology::TriangleList};
};

struct DepthStencilState
{
  bool      depth_test{false};
  bool      depth_write{false};
  CompareOp depth_compare{CompareOp::Less};

  bool stencil_test{false};
};

enum class BlendFactor : u8
{
  Zero,
  One,

  SrcColor,
  OneMinusSrcColor,
  DstColor,
  OneMinusDstColor,

  SrcAlpha,
  OneMinusSrcAlpha,
  DstAlpha,
  OneMinusDstAlpha,

  ConstantColor,
  OneMinusConstantColor,
  ConstantAlpha,
  OneMinusConstantAlpha,

  SrcAlphaSaturate,

  Src1Color,
  OneMinusSrc1Color,
  Src1Alpha,
  OneMinusSrc1Alpha,
};

enum class BlendOp : u8
{
  Add,
  Subtract,
  ReverseSubtract,
  Min,
  Max,
};

enum class ColorWrite : u8
{
  None = 0,
  R    = 1 << 0,
  G    = 1 << 1,
  B    = 1 << 2,
  A    = 1 << 3,
  RGBA = R | G | B | A
};

struct ColorBlendState
{
  bool enabled{false};

  BlendFactor src_color{BlendFactor::One};
  BlendFactor dst_color{BlendFactor::Zero};
  BlendOp     color_op{BlendOp::Add};

  BlendFactor src_alpha{BlendFactor::One};
  BlendFactor dst_alpha{BlendFactor::Zero};
  BlendOp     alpha_op{BlendOp::Add};

  ColorWrite write_mask{ColorWrite::RGBA};

  static constexpr auto opaque() noexcept -> ColorBlendState { return {}; }

  static constexpr auto alpha_blend() noexcept -> ColorBlendState
  {
    return {
        .enabled    = true,
        .src_color  = BlendFactor::SrcAlpha,
        .dst_color  = BlendFactor::OneMinusSrcAlpha,
        .color_op   = BlendOp::Add,
        .src_alpha  = BlendFactor::One,
        .dst_alpha  = BlendFactor::OneMinusSrcAlpha,
        .alpha_op   = BlendOp::Add,
        .write_mask = ColorWrite::RGBA,
    };
  }
};

namespace graphics {
struct GraphicsConfig
{
  std::vector<ShaderInfo> shader;
  RasterizationState      rasterization{};
  VertexInputState        vertex_input_state{};
  InputAssemblyState      input_assembly{};
  DepthStencilState       depth_stencil{};
  ColorBlendState         color_blend{};
  Format                  color_format{Format::RGBA8_UNORM};
  Format                  depth_format{Format::Undefined};
};

};  // namespace graphics

};  // namespace wind::vulkan
