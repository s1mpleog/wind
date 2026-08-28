#include "Resources/Builtin.hpp"

#include "Resources/ResourceManager.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Graphics/PipelineConfig.hpp"
#include "Vulkan/Graphics/PipelineManager.hpp"
#include "Vulkan/Graphics/ShaderTypes.hpp"
#include "glm/ext/vector_float4.hpp"

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace
{
auto SetupDefaultCube(UResourceManager *ResourceManager, UPipelineManager *PipelineManager,
                      const vk::raii::Device &Device) WIND_NOEXCEPT -> TWindResult<FBuiltAssets>
{
	auto CubeVert = WIND_TRY(ResourceManager->CreateVertexBuffer(std::as_bytes(std::span{CubeVertices})));

	auto CubeIndc = WIND_TRY(ResourceManager->CreateIndexBuffer(std::as_bytes(std::span{CubeIndices})));

	auto CubeVertexHandle = WIND_TRY(ResourceManager->LoadShader(Device, "assets/shaders/triangle.vert.spv"));

	auto CubeFragmentHandle = WIND_TRY(ResourceManager->LoadShader(Device, "assets/shaders/triangle.frag.spv"));

	FShaderInfo CubeVertShaderInfo{
	    .Stage = EShaderStage::Vertex,
	    .Module = WIND_TRY(ResourceManager->GetShader(CubeVertexHandle)),
	};

	FShaderInfo CubeFragShaderInfo{
	    .Stage = EShaderStage::Fragment,
	    .Module = WIND_TRY(ResourceManager->GetShader(CubeFragmentHandle)),
	};

	auto CubeConfig = FGraphicsConfig{
	    .Shader = {CubeVertShaderInfo, CubeFragShaderInfo},
	    .Rasterization =
	        {
	            .CullMode = ECullMode::None,
	        },
	    .VertexInputState =
	        {.Attributes =
	             {{.Location = 0, .Binding = 0, .Format = EVertexFormat::Float3, .Offset = offsetof(FVertex, Position)},
	              {.Location = 1, .Binding = 0, .Format = EVertexFormat::Float3, .Offset = offsetof(FVertex, Color)}},

	         .Bindings = {{
	             .Binding = 0,
	             .Stride = sizeof(FVertex),
	             .InputRate = EVertexInputRate::Vertex,
	         }}},
	    .ColorBlend = {.Enabled = false},
	    .PushConstants = {{
	        .StageFlags = EShaderStage::Vertex,
	        .Offset = 0,
	        .Size = sizeof(FPushConstants),
	    }},
	    .DescriptorSetLayout = std::nullopt,
	    .ColorFormat = EFormat::BgrA8Srgb};

	auto CubePipelineIndex = WIND_TRY(PipelineManager->Create(std::move(CubeConfig), Device));

	ResourceManager->DestroyShader(CubeVertexHandle);
	ResourceManager->DestroyShader(CubeFragmentHandle);

	auto AssetInfo = FBuiltAssets{.Pipelines = CubePipelineIndex,
	                              .HasDescriptorSets = false,
	                              .IsModel = false,

	                              .BufferAssets = FBufferAssets{.VertexHandle = CubeVert,
	                                                            .IndexHandle = CubeIndc,
	                                                            .IndexCount = static_cast<TU32>(CubeIndices.size()),
	                                                            .VertexCount = static_cast<TU32>(CubeVertices.size())}};
	return AssetInfo;
}

} // namespace

WIND_NODISCARD auto Build(UResourceManager *ResourceManager, UPipelineManager *PipelineManager,
                          const vk::raii::Device &Device) WIND_NOEXCEPT -> TWindResult<std::vector<FBuiltAssets>>
{

	auto VertexShaderHandle = WIND_TRY(ResourceManager->LoadShader(Device, "assets/shaders/model.vert.spv"));

	auto FragmentShaderHandle = WIND_TRY(ResourceManager->LoadShader(Device, "assets/shaders/model.frag.spv"));

	FShaderInfo VertInfo{
	    .Stage = EShaderStage::Vertex,
	    .Module = WIND_TRY(ResourceManager->GetShader(VertexShaderHandle)),
	};

	FShaderInfo FragInfo{
	    .Stage = EShaderStage::Fragment,
	    .Module = WIND_TRY(ResourceManager->GetShader(FragmentShaderHandle)),
	};

	auto SuzanneConfig = FGraphicsConfig{
	    .Shader = {VertInfo, FragInfo},
	    .Rasterization{
	        .CullMode = ECullMode::Back,
	        .PolygonMode = EPolygonMode::Fill,
	        .FrontFace = EFrontFace::CounterClockwise,
	        .DepthClamp = false,
	        .Discard = false,
	    },
	    .VertexInputState{
	        .Attributes{{
	                        .Location = 0,
	                        .Binding = 0,
	                        .Format = EVertexFormat::Float3,
	                        .Offset = 0,
	                    },
	                    {
	                        .Location = 1,
	                        .Binding = 1,
	                        .Format = EVertexFormat::Float3,
	                        .Offset = 0,
	                    },
	                    {
	                        .Location = 2,
	                        .Binding = 2,
	                        .Format = EVertexFormat::Float2,
	                        .Offset = 0,
	                    },
	                    {
	                        .Location = 3,
	                        .Binding = 3,
	                        .Format = EVertexFormat::Float4,
	                        .Offset = 0,
	                    }},
	        .Bindings{{
	                      .Binding = 0,
	                      .Stride = sizeof(glm::vec3),
	                      .InputRate = EVertexInputRate::Vertex,
	                  },
	                  {
	                      .Binding = 1,
	                      .Stride = sizeof(glm::vec3),
	                      .InputRate = EVertexInputRate::Vertex,
	                  },
	                  {
	                      .Binding = 2,
	                      .Stride = sizeof(glm::vec2),
	                      .InputRate = EVertexInputRate::Vertex,
	                  },
	                  {.Binding = 3, .Stride = sizeof(glm::vec4), .InputRate = EVertexInputRate::Vertex}},
	    },
	    .InputAssembly{.Topology = EPrimitiveTopology::TriangleList},
	    .DepthStencil{
	        .DepthTest = true,
	        .DepthWrite = true,
	        .DepthCompare = ECompareOp::Less,
	    },
	    .ColorBlend = {.Enabled = false},
	    .PushConstants{{
	        .StageFlags = EShaderStage::Vertex | EShaderStage::Fragment,
	        .Offset = 0,
	        .Size = sizeof(FPushConstants),
	    }},
	    .DescriptorSetLayout = *ResourceManager->GetBindlessDescriptorLayout(),
	    .ColorFormat = EFormat::BgrA8Srgb,
	    .DepthFormat = EFormat::D32Float};

	auto PipelineHandle = WIND_TRY(PipelineManager->Create(std::move(SuzanneConfig), Device));

	ResourceManager->DestroyShader(VertexShaderHandle);
	ResourceManager->DestroyShader(FragmentShaderHandle);

	auto ModelHandle = WIND_TRY(ResourceManager->LoadModel("assets/models/thanos.wind"));

	std::vector<FBuiltAssets> Assets;

	Assets.push_back(
	    FBuiltAssets{.Models = ModelHandle, .Pipelines = PipelineHandle, .HasDescriptorSets = true, .IsModel = true});

	auto Cube = WIND_TRY(SetupDefaultCube(ResourceManager, PipelineManager, Device));

	Assets.push_back(Cube);

	return Assets;
}
