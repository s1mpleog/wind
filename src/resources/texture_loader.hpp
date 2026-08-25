#pragma once

#include "config.hpp"
#include "error.hpp"
#include "glm/ext/vector_float3.hpp"
#include "spdlog/spdlog.h"
#include "utils/expected_util.hpp"
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

namespace wind::asset {

struct WindTexture
{
  u32             width{};
  u32             height{};
  u32             format{};
  std::vector<u8> data;
};

struct WindMaterial
{
  std::optional<u32>   albedo_index;
  std::optional<u32>   normal_index;
  std::optional<u32>   metallic_roughness_index;
  float                metallic{};
  float                roughness{};
  std::array<float, 4> base_color{};
};

struct WindAsset
{
  std::vector<glm::vec3>    vertices;
  std::vector<u32>          indices;
  std::vector<WindTexture>  textures;
  std::vector<WindMaterial> materials;
};

WIND_NODISCARD WIND_INLINE auto decode_wind_asset(std::span<const u8> buffer) WIND_NOEXCEPT -> WindResult<WindAsset>
{
  size_t cursor{0};

  std::array<const u8, 8> WIND_MAGIC{
      u8{87}, u8{73}, u8{78}, u8{68}, 0, 0, 0, 0,
  };

  auto magic = buffer.subspan(0, 8);

  if(!std::ranges::equal(magic, WIND_MAGIC))
  {
    WIND_ERR(WindError::internal());
  }

  cursor += magic.size();

  auto read_u32 = [](std::span<const u8> buf, size_t& cursor) -> u32 {
    u32 value{};
    std::memcpy(&value, buf.data() + cursor, sizeof(u32));
    cursor += sizeof(u32);
    return value;
  };

  auto read_u64 = [](std::span<const u8> buf, size_t& cursor) -> u64 {
    u64 value{};
    std::memcpy(&value, buf.data() + cursor, sizeof(u64));
    cursor += sizeof(u64);
    return value;
  };

  auto read_f32 = [](std::span<const u8> buf, size_t& cursor) -> float {
    float value{};
    std::memcpy(&value, buf.data() + cursor, sizeof(float));
    cursor += sizeof(float);
    return value;
  };

  // u32 version     = read_u32(buffer, cursor);
  cursor += sizeof(u32);
  u32 chunk_count = read_u32(buffer, cursor);

  if(chunk_count < 2)
    WIND_ERR(WindError::internal());

  constexpr u32 CHUNK_VERT = 0x54524556;  // "VERT"
  constexpr u32 CHUNK_INDC = 0x43444E49;  // "INDC"
  constexpr u32 CHUNK_TEXT = 0x54584554;  // "TEXT"
  constexpr u32 CHUNK_MATE = 0x4554414D;  // "MATE"
  constexpr u32 CHUNK_END  = 0x444E4549;  // "IEND"

  // at this point we have read the header now comes the data

  WindAsset asset{};

  for(u32 i = 0; i < chunk_count; i++)
  {
    auto type = read_u32(buffer, cursor);

    switch(type)
    {
      case CHUNK_VERT: {
        auto size     = read_u64(buffer, cursor);
        auto vertices = buffer.subspan(cursor, size);

        asset.vertices.resize(vertices.size());

        std::memcpy(asset.vertices.data(), vertices.data(), vertices.size());

        WIND_ASSERT(vertices.size() == asset.vertices.size() && "Memcpy failed");

        cursor += size;
        break;
      }

      case CHUNK_INDC: {
        auto size    = read_u64(buffer, cursor);
        auto indices = buffer.subspan(cursor, size);

        asset.indices.resize(indices.size());

        std::memcpy(asset.indices.data(), indices.data(), indices.size());

        WIND_ASSERT(indices.size() == asset.indices.size() && "Memcpy failed");

        cursor += size;
        break;
      }

      case CHUNK_TEXT: {
        auto width  = read_u32(buffer, cursor);
        auto height = read_u32(buffer, cursor);
        auto format = read_u32(buffer, cursor);

        WIND_ASSERT(format > 0 || format < 7 && "invalid texture format");

        auto size = read_u64(buffer, cursor);

        auto texture_data = buffer.subspan(cursor, size);

        auto texture = WindTexture{.width = width, .height = height, .format = format, .data{}};
        texture.data.resize(texture_data.size());

        std::memcpy(texture.data.data(), texture_data.data(), texture_data.size());

        WIND_ASSERT(texture.data.size() == texture_data.size() && "MEMCPY failed");

        asset.textures.push_back(std::move(texture));

        cursor += size;

        break;
      }

      case CHUNK_MATE: {
        // auto size = read_u64(buffer, cursor);
        cursor += sizeof(u64);
        // skip if index == U32::MAX that means there is no index
        auto albedo_index             = read_u32(buffer, cursor);
        auto normal_index             = read_u32(buffer, cursor);
        auto metallic_roughness_index = read_u32(buffer, cursor);

        auto metallic  = read_f32(buffer, cursor);
        auto roughness = read_f32(buffer, cursor);

        auto base_r = read_f32(buffer, cursor);
        auto base_g = read_f32(buffer, cursor);
        auto base_b = read_f32(buffer, cursor);
        auto base_a = read_f32(buffer, cursor);

        WIND_ASSERT(base_r > 0 || base_r < 1 || base_g > 0 || base_g < 1 || base_b > 0 || base_b < 1 || base_a > 0
                    || base_a < 1 && "invalid color");

        asset.materials.emplace_back(WindMaterial{
            .albedo_index = albedo_index == UINT32_MAX ? std::optional<u32>{std::nullopt} : std::optional{albedo_index},
            .normal_index = normal_index == UINT32_MAX ? std::optional<u32>{std::nullopt} : std::optional{normal_index},
            .metallic_roughness_index = metallic_roughness_index == UINT32_MAX ? std::optional<u32>{std::nullopt} :
                                                                                 std::optional{metallic_roughness_index},
            .metallic   = metallic,
            .roughness  = roughness,
            .base_color = {base_r, base_g, base_b, base_a},
        });

        break;
      }

      default:
        spdlog::info("unknown type: {}", type);
        break;
    }
  }

  // auto type = read_u32(buffer, cursor);

  // if(type != CHUNK_END)
  //   WIND_ERR(WindError::internal());

  return asset;
}

WIND_NODISCARD WIND_INLINE auto read_file(std::string_view name) WIND_NOEXCEPT -> WindResult<std::vector<u8>>
{
  auto path = std::filesystem::path(name);

  if(!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path))
    WIND_ERR(WindError::internal(ErrorCode::ResourceNotFound));

  auto file_size = std::filesystem::file_size(path);

  if(file_size == 0)
    WIND_ERR(WindError::internal());

  std::ifstream file_stream(path, std::ios::binary);

  if(!file_stream.is_open())
    WIND_ERR(WindError::internal());

  std::vector<u8> buffer(file_size);

  if(!file_stream.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(file_size)))
    WIND_ERR(WindError::internal());

  return buffer;
}

WIND_NODISCARD WIND_INLINE auto open(std::string_view name) WIND_NOEXCEPT -> WindResult<WindAsset>
{
  auto buffer = WIND_TRY(read_file(name));

  // spdlog::info("read {} bytes", buffer.size());

  auto asset = WIND_TRY(decode_wind_asset(buffer));

  spdlog::info("read vertex data: {} bytes, index data: {} bytes", asset.vertices.size(), asset.indices.size());

  if(!asset.textures.empty())
  {
    spdlog::info("total textures: {}", asset.textures.size());
    for(const auto& texture : asset.textures)
    {
      spdlog::info("width: {}, height: {}, format: {}, data length: {}", texture.width, texture.height, texture.format,
                   texture.data.size());
    }
  }

  if(!asset.materials.empty())
  {
    spdlog::info("total materials: {}", asset.materials.size());
    for(const auto& material : asset.materials)
    {
      spdlog::info("albedo: {}, normal: {}, metallic_roughness: {}, roughness: {}, metallic {}, base_color: {}, {}, {}, {}",
                   !material.albedo_index || material.albedo_index.value(),
                   !material.normal_index || material.normal_index.value(),
                   !material.metallic_roughness_index || material.metallic_roughness_index.value(), material.roughness,
                   material.metallic, material.base_color[0], material.base_color[1], material.base_color[2],
                   material.base_color[3]);
    }
  }

  return asset;
};

};  // namespace wind::asset
