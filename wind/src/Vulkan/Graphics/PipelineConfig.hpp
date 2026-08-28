#pragma once

#include "Types.hpp"

#include <vector>

enum class EPrimitiveTopology : uint8_t
{
	PointList,
	LineList,
	LineStrip,
	TriangleList,
	TriangleStrip,
	TriangleFan,
};

enum class ECullMode : u8
{
	Back,
	FontAndBack,
	None,
};

enum class EPolygonMode : u8
{
	Fill,
	Line,
	Point
};

enum class EFrontFace : u8
{
	CounterClockwise,
	ClockWise
};

enum class EFormat : uint16_t
{
	Undefined,

	// --- 8-bit Unsigned / Signed / Int Formats ---
	R8Unorm,
	R8Snorm,
	R8Uint,
	R8Sint,
	R8Srgb,

	RG8Unorm,
	RG8Snorm,
	RG8Uint,
	RG8Sint,
	RG8Srgb,

	RgbA8Unorm,
	RgbA8Snorm,
	RgbA8Uint,
	RgbA8Sint,
	RgbA8Srgb,

	BgrA8Unorm,
	BgrA8Srgb,

	// --- Packed 8/10/16-bit Normalized Formats ---
	RgB565UnormPacK16,
	RgbA4UnormPacK16,
	RgB5A1UnormPacK16,
	A2B10G10R10UnormPacK32,
	A2B10G10R10UintPacK32,
	A2R10G10B10UnormPacK32,
	B10G11R11UfloatPacK32,
	E5B9G9R9UfloatPacK32,

	// --- 16-bit Formats ---
	R16Unorm,
	R16Snorm,
	R16Uint,
	R16Sint,
	R16Float,

	RG16Unorm,
	RG16Snorm,
	RG16Uint,
	RG16Sint,
	RG16Float,

	RgbA16Unorm,
	RgbA16Snorm,
	RgbA16Uint,
	RgbA16Sint,
	RgbA16Float,

	// --- 32-bit Formats ---
	R32Uint,
	R32Sint,
	R32Float,

	RG32Uint,
	RG32Sint,
	RG32Float,

	RgB32Uint,
	RgB32Sint,
	RgB32Float,

	RgbA32Uint,
	RgbA32Sint,
	RgbA32Float,

	// --- Depth / Stencil Formats ---
	D16Unorm,
	X8D24UnormPacK32,
	D32Float,
	S8Uint,
	D16UnormS8Uint,
	D24UnormS8Uint,
	D32FloatS8Uint,

	// --- BC Compressed Textures (Desktop / Core) ---
	BC1RgbUnormBlock,
	BC1RgbSrgbBlock,
	BC1RgbaUnormBlock,
	BC1RgbaSrgbBlock,
	BC2UnormBlock,
	BC2SrgbBlock,
	BC3UnormBlock,
	BC3SrgbBlock,
	BC4UnormBlock,
	BC4SnormBlock,
	BC5UnormBlock,
	BC5SnormBlock,
	BC6HUfloatBlock,
	BC6HSfloatBlock,
	BC7UnormBlock,
	BC7SrgbBlock,

};

enum class EVertexFormat : u8
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

enum class EVertexInputRate : u8
{
	Vertex,
	Instance,
};

struct FVertexAttribute
{
	u32 Location{};
	u32 Binding{};
	EVertexFormat Format{};
	u32 Offset{};
};

struct FVertexBinding
{
	uint32_t Binding;
	uint32_t Stride;
	EVertexInputRate InputRate;
};

struct FVertexInputState
{
	std::vector<FVertexAttribute> Attributes;
	std::vector<FVertexBinding> Bindings;
};

enum class ECompareOp : u8
{
	Less,
	Always,
	Never,
	Greater
};

// enum class ShaderStage : u8
// {
//   Vertex,
//   Fragment,
//   Compute
// };

enum class EShaderStage : u8
{
	Vertex = 1 << 0,
	Fragment = 1 << 1,
	Compute = 1 << 2,
};

constexpr auto operator|(EShaderStage Lhs, EShaderStage Rhs) WIND_NOEXCEPT->EShaderStage
{
	using T_ = std::underlying_type_t<EShaderStage>;

	return static_cast<EShaderStage>(static_cast<T_>(Lhs) | static_cast<T_>(Rhs));
}

constexpr auto operator&(EShaderStage Lhs, EShaderStage Rhs) WIND_NOEXCEPT->EShaderStage
{
	using T_ = std::underlying_type_t<EShaderStage>;

	return static_cast<EShaderStage>(static_cast<T_>(Lhs) & static_cast<T_>(Rhs));
}

constexpr auto operator|=(EShaderStage &Lhs, EShaderStage Rhs) WIND_NOEXCEPT->EShaderStage &
{
	Lhs = Lhs | Rhs;
	return Lhs;
}

struct FShaderInfo
{
	EShaderStage Stage{EShaderStage::Vertex};
	std::string_view EntryPoint{"main"};
	const vk::raii::ShaderModule *Module{nullptr};
};

struct FRasterizationState
{
	ECullMode CullMode{ECullMode::Back};
	EPolygonMode PolygonMode{EPolygonMode::Fill};
	EFrontFace FrontFace{EFrontFace::ClockWise};

	bool DepthClamp{false};
	bool Discard{false};
};

struct FInputAssemblyState
{
	EPrimitiveTopology Topology{EPrimitiveTopology::TriangleList};
};

struct FDepthStencilState
{
	bool DepthTest{false};
	bool DepthWrite{false};
	ECompareOp DepthCompare{ECompareOp::Less};

	bool StencilTest{false};
};

enum class EBlendFactor : u8
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

enum class EBlendOp : u8
{
	Add,
	Subtract,
	ReverseSubtract,
	Min,
	Max,
};

enum class EColorWrite : u8
{
	None = 0,
	R = 1 << 0,
	G = 1 << 1,
	B = 1 << 2,
	A = 1 << 3,
	RGBA = R | G | B | A
};

struct FColorBlendState
{
	bool Enabled{false};

	EBlendFactor SrcColor{EBlendFactor::One};
	EBlendFactor DstColor{EBlendFactor::Zero};
	EBlendOp ColorOp{EBlendOp::Add};

	EBlendFactor SrcAlpha{EBlendFactor::One};
	EBlendFactor DstAlpha{EBlendFactor::Zero};
	EBlendOp AlphaOp{EBlendOp::Add};

	EColorWrite WriteMask{EColorWrite::RGBA};

	static constexpr auto Opaque() WIND_NOEXCEPT -> FColorBlendState
	{
		return {};
	}

	static constexpr auto AlphaBlend() WIND_NOEXCEPT -> FColorBlendState
	{
		return {
		    .Enabled = true,
		    .SrcColor = EBlendFactor::SrcAlpha,
		    .DstColor = EBlendFactor::OneMinusSrcAlpha,
		    .ColorOp = EBlendOp::Add,
		    .SrcAlpha = EBlendFactor::One,
		    .DstAlpha = EBlendFactor::OneMinusSrcAlpha,
		    .AlphaOp = EBlendOp::Add,
		    .WriteMask = EColorWrite::RGBA,
		};
	}
};

struct FPushConstantRange
{
	// temporary introduce custom type
	EShaderStage StageFlags{EShaderStage::Vertex};
	u32 Offset{};
	u32 Size{};
};

struct FGraphicsConfig
{
	std::vector<FShaderInfo> Shader;
	FRasterizationState Rasterization{};
	FVertexInputState VertexInputState{};
	FInputAssemblyState InputAssembly{};
	FDepthStencilState DepthStencil{};
	FColorBlendState ColorBlend{};
	std::vector<FPushConstantRange> PushConstants;
	std::optional<vk::DescriptorSetLayout> DescriptorSetLayout;
	EFormat ColorFormat{EFormat::RgbA8Unorm};
	EFormat DepthFormat{EFormat::Undefined};
};
