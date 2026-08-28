#include "Resources/ResourceManager.hpp"

#include "Config.hpp"
#include "Error.hpp"
#include "Resources/DescriptorManager.hpp"
#include "Resources/TextureLoader.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Graphics/PipelineConfig.hpp"
#include "Vulkan/Memory/Allocator.hpp"
#include "Vulkan/Memory/ResourceTypes.hpp"
#include "spdlog/spdlog.h"

#include <array>
#include <inplace_vector>
#include <ranges>
#include <span>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

WIND_NODISCARD auto UResourceManager::Create(const FVulkanContext *Context) WIND_NOEXCEPT
    -> WindResult<UResourceManager>
{
	auto Allocator = WIND_TRY(UGpuAllocator::Create(Context));

	auto DescriptorManager =
	    WIND_TRY(UDescriptorManager::Create(Context->GpuDevice.Device, vk::DescriptorType::eCombinedImageSampler));

	std::inplace_vector<vk::DescriptorSetLayoutBinding, 2> Bindings{
	    vk::DescriptorSetLayoutBinding{
	        0,
	        vk::DescriptorType::eCombinedImageSampler,
	        1024,
	        vk::ShaderStageFlagBits::eFragment,
	    },
	    vk::DescriptorSetLayoutBinding{
	        1,
	        vk::DescriptorType::eUniformBufferDynamic,
	        1,
	        vk::ShaderStageFlagBits::eVertex,
	    },
	};

	std::array<vk::DescriptorBindingFlags, 2> BindingFlags{
	    vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateAfterBind,

	    vk::DescriptorBindingFlags{} // binding 1: dynamic UBO
	};

	vk::DescriptorSetLayoutBindingFlagsCreateInfo FlagsInfo{};
	FlagsInfo.bindingCount = BindingFlags.size();
	FlagsInfo.pBindingFlags = BindingFlags.data();
	vk::DescriptorSetLayoutCreateInfo LayoutInfo{};

	LayoutInfo.pNext = &FlagsInfo;
	LayoutInfo.flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;
	LayoutInfo.bindingCount = Bindings.size();
	LayoutInfo.pBindings = Bindings.data();

	WIND_TRY(DescriptorManager.CreateLayout(Context->GpuDevice.Device, std::span{Bindings}));
	WIND_TRY(DescriptorManager.CreateSet(Context->GpuDevice.Device));

	return UResourceManager{Context, std::move(Allocator), std::move(DescriptorManager)};
}

WIND_NODISCARD auto UResourceManager::LoadShader(const vk::raii::Device &Device,
                                                 std::string_view ShaderPath) WIND_NOEXCEPT -> WindResult<TShaderHandle>
{
	auto It = MShaderCache.find(std::string{ShaderPath});

	spdlog::info("loading shader: {}", ShaderPath);

	if (It != MShaderCache.end())
		return It->second;

	// cache miss create new shader
	auto Path = std::filesystem::path(ShaderPath);

	if (!std::filesystem::exists(Path) || !std::filesystem::is_regular_file(Path))
		WIND_ERR(WindError::internal(ErrorCode::FailedToLoadShader));

	auto FileSize = std::filesystem::file_size(Path);

	if (FileSize == 0)
		WIND_ERR(WindError::internal(ErrorCode::FailedToLoadShader));

	if (FileSize % sizeof(u32) != 0)
		WIND_ERR(WindError::internal());

	std::ifstream FileStream(Path, std::ios::binary);

	if (!FileStream.is_open())
		WIND_ERR(WindError::internal());

	std::vector<u32> Buffer(FileSize / sizeof(u32));

	if (!FileStream.read(reinterpret_cast<char *>(Buffer.data()), static_cast<std::streamsize>(FileSize)))
		WIND_ERR(WindError::internal());

	vk::ShaderModuleCreateInfo ShaderModuleInfo{};
	ShaderModuleInfo.codeSize = Buffer.size() * sizeof(u32);
	ShaderModuleInfo.pCode = Buffer.data();

	u32 Index = static_cast<u32>(MShaders.size());

	MShaders.emplace_back(WIND_TRY(Device.createShaderModule(ShaderModuleInfo)));

	TShaderHandle Handle{.Index = Index};

	MShaderCache.emplace(std::string{ShaderPath}, Handle);

	return Handle;
}

WIND_NODISCARD auto UResourceManager::GetShader(TShaderHandle Handle) WIND_NOEXCEPT
    -> WindResult<vk::raii::ShaderModule *>
{
	if (Handle.Index >= MShaders.size())
		WIND_ERR(WindError::internal());

	return &MShaders[Handle.Index];
};

WIND_NODISCARD auto UResourceManager::GetShaderUnchecked(TShaderHandle Handle) WIND_NOEXCEPT -> vk::raii::ShaderModule *
{
	return &MShaders[Handle.Index];
}

// TODO: handle this in better way
auto UResourceManager::DestroyShader(TShaderHandle Handle) WIND_NOEXCEPT -> void
{
	if (Handle.Index > MShaders.size())
		return;

	MShaders[Handle.Index] = nullptr;
}

WIND_NODISCARD auto UResourceManager::LoadModel(std::string_view TexturePath) WIND_NOEXCEPT -> WindResult<TModelHandle>
{
	auto It = MModelCache.find(std::string{TexturePath});

	if (It != MModelCache.end())
	{
		spdlog::info("model is already in cache loading");
		return It->second;
	}

	auto Model = FModel{};

	auto WindAsset = WIND_TRY(Open(TexturePath));

	for (const auto &SubMesh : WindAsset.Mesh.SubMeshes)
	{
		Model.Mesh.SubMeshes.emplace_back(FSubMesh{
		    .IndexCount = SubMesh.IndexCount,
		    .IndexOffset = SubMesh.IndexOffset,
		    .MaterialIndex = SubMesh.MaterialIndex,
		});
	}

	// TODO: instead of two buffers use one buffer
	// |---------------------------|
	// vertex           index

	// mesh.index_buffer = WIND_TRY(m_allocator.create_buffer(m_context, ));

	std::inplace_vector<FBufferData, 5> Buffers{{FBufferData{
	                                                .Data = std::as_bytes(std::span{WindAsset.Mesh.Position}),
	                                                .Usage = vk::BufferUsageFlagBits::eVertexBuffer,
	                                            }},
	                                            {FBufferData{
	                                                .Data = std::as_bytes(std::span{WindAsset.Mesh.Indices}),
	                                                .Usage = vk::BufferUsageFlagBits::eIndexBuffer,
	                                            }},
	                                            {FBufferData{
	                                                .Data = std::as_bytes(std::span{WindAsset.Mesh.Normals}),
	                                                .Usage = vk::BufferUsageFlagBits::eVertexBuffer,
	                                            }},
	                                            {FBufferData{
	                                                .Data = std::as_bytes(std::span{WindAsset.Mesh.Uvs}),
	                                                .Usage = vk::BufferUsageFlagBits::eVertexBuffer,
	                                            }},
	                                            {FBufferData{
	                                                .Data = std::as_bytes(std::span{WindAsset.Mesh.Tangents}),
	                                                .Usage = vk::BufferUsageFlagBits::eVertexBuffer,
	                                            }}};

	// TODO: currently .wind fallbacks for uvs and normals so it will always exists
	//  but later its not true so handle that with this
	//  vertices and indices must exists according to .wind specs
	//  buffers.push_back({
	//      .data  = std::as_bytes(std::span{wind_asset.mesh.Position}),
	//      .usage = vk::BufferUsageFlagBits::eVertexBuffer,
	//  });

	// buffers.push_back({
	//     .data  = std::as_bytes(std::span{wind_asset.mesh.indices}),
	//     .usage = vk::BufferUsageFlagBits::eIndexBuffer,
	// });

	// // normals and uvs can be empty
	// if(!wind_asset.mesh.normals.empty())
	// {
	//   buffers.push_back({
	//       .data  = std::as_bytes(std::span{wind_asset.mesh.normals}),
	//       .usage = vk::BufferUsageFlagBits::eVertexBuffer,
	//   });
	// }

	// if(!wind_asset.mesh.uvs.empty())
	// {
	//   buffers.push_back({
	//       .data  = std::as_bytes(std::span{wind_asset.mesh.uvs}),
	//       .usage = vk::BufferUsageFlagBits::eVertexBuffer,
	//   });
	// }

	auto Result = WIND_TRY(MAllocator.CreateBuffers(MContext, Buffers));

	Model.Mesh.VertexBuffer = std::move(Result[0]);
	Model.Mesh.IndexBuffer = std::move(Result[1]);
	Model.Mesh.Normals = std::move(Result[2]);
	Model.Mesh.Uvs = std::move(Result[3]);
	Model.Mesh.Tangents = std::move(Result[4]);

	Model.Mesh.VertexCount = static_cast<u32>(WindAsset.Mesh.Position.size());
	Model.Mesh.IndexCount = static_cast<u32>(WindAsset.Mesh.Indices.size());
	Model.Mesh.UvCount = static_cast<u32>(WindAsset.Mesh.Uvs.size());
	Model.Mesh.NormalCount = static_cast<u32>(WindAsset.Mesh.Normals.size());
	Model.Mesh.TangentCount = static_cast<u32>(WindAsset.Mesh.Tangents.size());

	// auto mesh_index = static_cast<u32>(m_meshes.size());

	// m_meshes.push_back(std::move(mesh));

	spdlog::info("buffer created successfully");

	// temporary
	auto Format = [](u32 Format) -> EFormat
	{
		switch (Format)
		{
		case 7:
			return EFormat::BC7UnormBlock;
		case 5:
			return EFormat::BC5UnormBlock;

		default:
			return EFormat::RgbA32Float;
		}
	};

	// only if texture and material is not empty
	MTexture.reserve(WindAsset.Textures.size());

	std::vector<FTextureData> TextureData;
	TextureData.reserve(WindAsset.Textures.size());

	for (const auto &&[index, texture] : std::views::enumerate(WindAsset.Textures))
	{
		auto Data = FTextureData{
		    .Pixels = std::as_bytes(std::span{texture.Data}),
		    .Dimensions = {texture.Width, texture.Height, 1},
		    .Format = Format(texture.Format),
		};

		TextureData.push_back(Data);
	}

	MTexture = WIND_TRY(MAllocator.CreateTexture(MContext, TextureData));

	spdlog::info("created textures: {}", MTexture.size());

	Model.Materials.resize(WindAsset.Materials.size());

	for (auto &&[index, material] : std::views::enumerate(WindAsset.Materials))
	{
		Model.Materials[index].Roughness = material.Roughness;
		Model.Materials[index].BaseColor = {material.BaseColor[0], material.BaseColor[1], material.BaseColor[2],
		                                    material.BaseColor[3]};

		if (material.AlbedoIndex)
		{
			auto &Texture = MTexture[*material.AlbedoIndex];
			Model.Materials[index].AlbedoTexture =
			    MDescriptorManager.RegisterTexture(MContext->GpuDevice.Device, Texture);
		}

		if (material.NormalIndex)
		{
			auto &Texture = MTexture[*material.NormalIndex];
			Model.Materials[index].NormalTexture =
			    MDescriptorManager.RegisterTexture(MContext->GpuDevice.Device, Texture);
		}

		if (material.MetallicRoughnessIndex)
		{
			auto &Texture = MTexture[*material.MetallicRoughnessIndex];
			Model.Materials[index].MetallicRoughnessTexture =
			    MDescriptorManager.RegisterTexture(MContext->GpuDevice.Device, Texture);
		}
	}

	u32 ModelIndex = static_cast<u32>(MModels.size());

	auto Handle = TModelHandle{.Index = ModelIndex};

	// update cache
	MModelCache.emplace(std::string{TexturePath}, Handle);

	MModels.push_back(std::move(Model));

	return Handle;
}

WIND_NODISCARD auto UResourceManager::CreateDefaultDepthImage(u32 Width, u32 Height) WIND_NOEXCEPT -> WindResult<void>
{
	MDepthImage = WIND_TRY(MAllocator.CreateDepthBuffer(MContext, Width, Height));
	return {};
}

WIND_NODISCARD auto UResourceManager::GetDefaultDepthImageView() const WIND_NOEXCEPT -> const vk::raii::ImageView &
{
	WIND_ASSERT(MDepthImage.ImageView != nullptr && "Depth image view is nullptr create depth image first");
	return MDepthImage.ImageView;
}

WIND_NODISCARD auto UResourceManager::GetModel(TModelHandle Handle) WIND_NOEXCEPT -> WindResult<const FModel *>
{
	if (Handle.Index >= MModels.size())
		WIND_ERR(WindError::internal());

	return &MModels[Handle.Index];
}

WIND_NODISCARD auto UResourceManager::GetModelUnchecked(TModelHandle Handle) WIND_NOEXCEPT -> const FModel *
{
	return &MModels[Handle.Index];
}

WIND_NODISCARD auto UResourceManager::RecreateDefaultDepthImage(u32 Width, u32 Height) WIND_NOEXCEPT -> WindResult<void>
{
	// destroy the old depth image
	MDepthImage.DestroyImage();

	// create new one
	MDepthImage = WIND_TRY(MAllocator.CreateDepthBuffer(MContext, Width, Height));

	return {};
}

WIND_NODISCARD auto UResourceManager::CreateVertexBuffer(std::span<const std::byte> Vertices) WIND_NOEXCEPT
    -> WindResult<TBufferHandle>
{
	auto Index = static_cast<u32>(MBuffers.size());

	MBuffers.emplace_back(WIND_TRY(MAllocator.CreateBuffer(MContext, FBufferData{
	                                                                     .Data = Vertices,
	                                                                 })));

	return TBufferHandle{.Index = Index};
}

WIND_NODISCARD auto UResourceManager::CreateIndexBuffer(std::span<const std::byte> Indices) -> WindResult<TBufferHandle>
{
	auto Index = static_cast<u32>(MBuffers.size());

	MBuffers.emplace_back(WIND_TRY(MAllocator.CreateBuffer(
	    MContext, FBufferData{.Data = Indices, .Usage = vk::BufferUsageFlagBits::eIndexBuffer})));

	return TBufferHandle{.Index = Index};
}

WIND_NODISCARD auto UResourceManager::GetBuffer(TBufferHandle Handle) const -> WindResult<const FAllocatedBuffer *>
{
	if (Handle.Index >= MBuffers.size())
		WIND_ERR(WindError::internal());

	return &MBuffers[Handle.Index];
}
WIND_NODISCARD auto UResourceManager::GetBufferUnchecked(TBufferHandle Handle) const -> const FAllocatedBuffer *
{
	return GetBuffer(Handle).value();
}

WIND_NODISCARD auto UResourceManager::CreateDynamicBuffer(u32 Size, vk::BufferUsageFlagBits Usage) WIND_NOEXCEPT
    -> WindResult<TDynamicBufferHandle>
{
	auto Handle = static_cast<u32>(MBuffers.size());
	auto Test = WIND_TRY(MAllocator.CreateDynamicBuffer(Size, Usage));
	spdlog::info("mapped is: {}", Test.Mapped);
	MBuffers.push_back(std::move(Test));
	return TDynamicBufferHandle{.Index = Handle};
}

WIND_NODISCARD auto UResourceManager::GetMappedData(TDynamicBufferHandle Handle) WIND_NOEXCEPT -> WindResult<void *>
{
	if (Handle.Index >= MBuffers.size())
		WIND_ERR(WindError::internal());

	if (MBuffers[Handle.Index].Mapped == nullptr)
		WIND_ERR(WindError::internal());

	return MBuffers[Handle.Index].Mapped;
}

WIND_NODISCARD auto UResourceManager::GetMappedDataUnchecked(TDynamicBufferHandle Handle) WIND_NOEXCEPT -> void *
{
	return MBuffers[Handle.Index].Mapped;
}

WIND_NODISCARD auto UResourceManager::CreateDynamicUniformBuffer(u32 Size) WIND_NOEXCEPT
    -> WindResult<TDynamicBufferHandle>
{
	auto Handle = WIND_TRY(CreateDynamicBuffer(Size, vk::BufferUsageFlagBits::eUniformBuffer));
	return Handle;
}
