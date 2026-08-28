#pragma once

#include "Config.hpp"
#include "Error.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

struct FWindTexture
{
	TU32 Width{};
	TU32 Height{};
	TU32 Format{};
	std::vector<TU8> Data;
};

struct FWindMaterial
{
	std::optional<TU32> AlbedoIndex;
	std::optional<TU32> NormalIndex;
	std::optional<TU32> MetallicRoughnessIndex;
	float Metallic{};
	float Roughness{};
	std::array<float, 4> BaseColor{};
};

struct FWindSubMesh
{
	TU32 IndexCount{};
	TU32 IndexOffset{};
	TU32 MaterialIndex{};
};

struct FWindMesh
{
	std::vector<glm::vec3> Position;
	std::vector<TU32> Indices;
	std::vector<glm::vec2> Uvs;
	std::vector<glm::vec3> Normals;
	std::vector<glm::vec4> Tangents;
	std::vector<FWindSubMesh> SubMeshes;
};

struct FWindAsset
{
	FWindMesh Mesh;
	std::vector<FWindTexture> Textures;
	std::vector<FWindMaterial> Materials;
};

WIND_NODISCARD WIND_INLINE auto DecodeWindAsset(std::span<const TU8> Buffer) WIND_NOEXCEPT -> TWindResult<FWindAsset>
{
	size_t Cursor{0};

	std::array<const TU8, 8> WindMagic{
	    TU8{87}, TU8{73}, TU8{78}, TU8{68}, 0, 0, 0, 0,
	};

	auto Magic = Buffer.subspan(0, 8);

	if (!std::ranges::equal(Magic, WindMagic))
	{
		WIND_ERR(WindError::internal());
	}

	Cursor += Magic.size();

	auto ReadU32 = [](std::span<const TU8> Buf, size_t &Cursor) -> TU32
	{
		TU32 Value{};
		std::memcpy(&Value, Buf.data() + Cursor, sizeof(TU32));
		Cursor += sizeof(TU32);
		return Value;
	};

	auto ReadU64 = [](std::span<const TU8> Buf, size_t &Cursor) -> TU64
	{
		TU64 Value{};
		std::memcpy(&Value, Buf.data() + Cursor, sizeof(TU64));
		Cursor += sizeof(TU64);
		return Value;
	};

	auto ReadF32 = [](std::span<const TU8> Buf, size_t &Cursor) -> float
	{
		float Value{};
		std::memcpy(&Value, Buf.data() + Cursor, sizeof(float));
		Cursor += sizeof(float);
		return Value;
	};

	// TU32 version     = read_u32(buffer, cursor);
	Cursor += sizeof(TU32);
	TU32 ChunkCount = ReadU32(Buffer, Cursor);

	if (ChunkCount < 2)
		WIND_ERR(WindError::internal());

	constexpr TU32 ChunkVert = 0x54524556; // "VERT"
	constexpr TU32 ChunkIndc = 0x43444E49; // "INDC"
	constexpr TU32 ChunkIuv = 0x5F565549;  // "IUV_"
	constexpr TU32 ChunkInor = 0x524F4E49; // "INOR"
	constexpr TU32 ChunkItan = 0x4E415449; // "ITAN"
	constexpr TU32 ChunkIsub = 0x42555349; // "ISUB"
	constexpr TU32 ChunkText = 0x54584554; // "TEXT"
	constexpr TU32 ChunkMate = 0x4554414D; // "MATE"
	constexpr TU32 ChunkEnd = 0x444E4549;  // "IEND"

	// at this point we have read the header now comes the data

	FWindAsset Asset{};

	for (TU32 I = 0; I < ChunkCount; I++)
	{
		auto Type = ReadU32(Buffer, Cursor);

		switch (Type)
		{
		case ChunkVert:
		{
			auto Size = ReadU64(Buffer, Cursor);
			auto Vertices = Buffer.subspan(Cursor, Size);

			Asset.Mesh.Position.resize(Vertices.size());

			std::memcpy(Asset.Mesh.Position.data(), Vertices.data(), Vertices.size());

			WIND_ASSERT(Vertices.size() == Asset.Mesh.Position.size() && "Memcpy failed");

			Cursor += Size;
			break;
		}

		case ChunkIndc:
		{
			auto Size = ReadU64(Buffer, Cursor);
			auto Indices = Buffer.subspan(Cursor, Size);

			Asset.Mesh.Indices.resize(Indices.size());

			std::memcpy(Asset.Mesh.Indices.data(), Indices.data(), Indices.size());

			WIND_ASSERT(Indices.size() == Asset.Mesh.Indices.size() && "Memcpy failed");

			Cursor += Size;
			break;
		}

		case ChunkIuv:
		{
			auto Size = ReadU64(Buffer, Cursor);
			auto Uvs = Buffer.subspan(Cursor, Size);

			spdlog::info("found uvs: {}", Uvs.size());

			Asset.Mesh.Uvs.resize(Uvs.size());
			std::memcpy(Asset.Mesh.Uvs.data(), Uvs.data(), Uvs.size());

			WIND_ASSERT(Uvs.size() == Asset.Mesh.Uvs.size() && "Memcpy failed");

			Cursor += Size;
			break;
		}

		case ChunkInor:
		{
			auto Size = ReadU64(Buffer, Cursor);
			auto Normals = Buffer.subspan(Cursor, Size);

			spdlog::info("found normals: {}", Normals.size());

			Asset.Mesh.Normals.resize(Normals.size());
			std::memcpy(Asset.Mesh.Normals.data(), Normals.data(), Normals.size());

			WIND_ASSERT(Normals.size() == Asset.Mesh.Normals.size() && "Memcpy failed");

			Cursor += Size;
			break;
		}

		case ChunkItan:
		{
			TU64 Size = ReadU64(Buffer, Cursor);
			auto Tangent = Buffer.subspan(Cursor, Size);

			Asset.Mesh.Tangents.resize(Tangent.size());

			std::memcpy(Asset.Mesh.Tangents.data(), Tangent.data(), Tangent.size());

			WIND_ASSERT(Tangent.size() == Asset.Mesh.Tangents.size() && "Memcpy failed");

			Cursor += Size;

			break;
		}

		case ChunkIsub:
		{
			// auto size = read_u64(buffer, cursor);
			Cursor += sizeof(TU64);
			// spdlog::info("sub_mesh size: {}", size);
			Asset.Mesh.SubMeshes.emplace_back(FWindSubMesh{.IndexCount = ReadU32(Buffer, Cursor),
			                                               .IndexOffset = ReadU32(Buffer, Cursor),
			                                               .MaterialIndex = ReadU32(Buffer, Cursor)});
			break;
		}

		case ChunkText:
		{
			auto Width = ReadU32(Buffer, Cursor);
			auto Height = ReadU32(Buffer, Cursor);
			auto Format = ReadU32(Buffer, Cursor);

			WIND_ASSERT(Format > 0 || Format < 7 && "invalid texture format");

			auto Size = ReadU64(Buffer, Cursor);

			auto TextureData = Buffer.subspan(Cursor, Size);

			auto Texture = FWindTexture{.Width = Width, .Height = Height, .Format = Format, .Data{}};
			Texture.Data.resize(TextureData.size());

			std::memcpy(Texture.Data.data(), TextureData.data(), TextureData.size());

			WIND_ASSERT(Texture.Data.size() == TextureData.size() && "MEMCPY failed");

			Asset.Textures.push_back(std::move(Texture));

			Cursor += Size;

			break;
		}

		case ChunkMate:
		{
			// auto size = read_u64(buffer, cursor);
			Cursor += sizeof(TU64);
			// skip if index == U32::MAX that means there is no index
			auto AlbedoIndex = ReadU32(Buffer, Cursor);
			auto NormalIndex = ReadU32(Buffer, Cursor);
			auto MetallicRoughnessIndex = ReadU32(Buffer, Cursor);

			auto Metallic = ReadF32(Buffer, Cursor);
			auto Roughness = ReadF32(Buffer, Cursor);

			auto BaseR = ReadF32(Buffer, Cursor);
			auto BaseG = ReadF32(Buffer, Cursor);
			auto BaseB = ReadF32(Buffer, Cursor);
			auto BaseA = ReadF32(Buffer, Cursor);

			WIND_ASSERT(BaseR > 0 || BaseR < 1 || BaseG > 0 || BaseG < 1 || BaseB > 0 || BaseB < 1 || BaseA > 0 ||
			            BaseA < 1 && "invalid color");

			Asset.Materials.emplace_back(FWindMaterial{
			    .AlbedoIndex =
			        AlbedoIndex == UINT32_MAX ? std::optional<TU32>{std::nullopt} : std::optional{AlbedoIndex},
			    .NormalIndex =
			        NormalIndex == UINT32_MAX ? std::optional<TU32>{std::nullopt} : std::optional{NormalIndex},
			    .MetallicRoughnessIndex = MetallicRoughnessIndex == UINT32_MAX ? std::optional<TU32>{std::nullopt}
			                                                                   : std::optional{MetallicRoughnessIndex},
			    .Metallic = Metallic,
			    .Roughness = Roughness,
			    .BaseColor = {BaseR, BaseG, BaseB, BaseA},
			});

			break;
		}

		default:
			spdlog::info("unknown type: {}", Type);
			break;
		}
	}

	auto Type = ReadU32(Buffer, Cursor);

	if (Type != ChunkEnd)
		WIND_ERR(WindError::internal());

	return Asset;
}

WIND_NODISCARD WIND_INLINE auto ReadFile(std::string_view Name) WIND_NOEXCEPT -> TWindResult<std::vector<TU8>>
{
	auto Path = std::filesystem::path(Name);

	if (!std::filesystem::exists(Path) || !std::filesystem::is_regular_file(Path))
		WIND_ERR(WindError::internal(ErrorCode::ResourceNotFound));

	auto FileSize = std::filesystem::file_size(Path);

	if (FileSize == 0)
		WIND_ERR(WindError::internal());

	std::ifstream FileStream(Path, std::ios::binary);

	if (!FileStream.is_open())
		WIND_ERR(WindError::internal());

	std::vector<TU8> Buffer(FileSize);

	if (!FileStream.read(reinterpret_cast<char *>(Buffer.data()), static_cast<std::streamsize>(FileSize)))
		WIND_ERR(WindError::internal());

	return Buffer;
}

WIND_NODISCARD WIND_INLINE auto Open(std::string_view Name) WIND_NOEXCEPT -> TWindResult<FWindAsset>
{
	auto Buffer = WIND_TRY(ReadFile(Name));

	// spdlog::info("read {} bytes", buffer.size());

	auto Asset = WIND_TRY(DecodeWindAsset(Buffer));

	spdlog::info("read vertex data: {} bytes, index data: {} bytes", Asset.Mesh.Position.size(),
	             Asset.Mesh.Indices.size());

	if (!Asset.Textures.empty())
	{
		spdlog::info("total textures: {}", Asset.Textures.size());
		for (const auto &Texture : Asset.Textures)
		{
			spdlog::info("width: {}, height: {}, format: {}, data length: {}", Texture.Width, Texture.Height,
			             Texture.Format, Texture.Data.size());
		}
	}

	if (!Asset.Materials.empty())
	{
		spdlog::info("total materials: {}", Asset.Materials.size());
		for (const auto &Material : Asset.Materials)
		{
			spdlog::info("albedo: {}, normal: {}, metallic_roughness: {}, roughness: {}, metallic {}, base_color: {}, "
			             "{}, {}, {}",
			             !Material.AlbedoIndex || (Material.AlbedoIndex.value() != 0U),
			             !Material.NormalIndex || (Material.NormalIndex.value() != 0U),
			             !Material.MetallicRoughnessIndex || (Material.MetallicRoughnessIndex.value() != 0U),
			             Material.Roughness, Material.Metallic, Material.BaseColor[0], Material.BaseColor[1],
			             Material.BaseColor[2], Material.BaseColor[3]);
		}
	}

	if (!Asset.Mesh.SubMeshes.empty())
	{
		spdlog::info("read: {} sub_meshes", Asset.Mesh.SubMeshes.size());
		for (const auto &Submesh : Asset.Mesh.SubMeshes)
		{
			spdlog::info("sub_mesh -> index_count: {}, index_offset: {}, material_index: {}", Submesh.IndexCount,
			             Submesh.IndexOffset, Submesh.MaterialIndex);
		}
	}

	return Asset;
};
