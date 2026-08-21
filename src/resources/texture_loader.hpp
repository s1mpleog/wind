#pragma once

#include "config.hpp"
#include "error.hpp"
#include "glm/ext/vector_float3.hpp"
#include "spdlog/spdlog.h"
#include "utils/expected_util.hpp"
#include <algorithm>
#include <array>
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
  u8              format{};
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

WIND_NODISCARD WIND_INLINE auto decode(std::span<const u8> buffer) WIND_NOEXCEPT -> WindResult<WindAsset>
{
  size_t cursor{0};

  std::array<const u8, 8> VALID_MAGIC{
      u8{87}, u8{73}, u8{78}, u8{68}, 0, 0, 0, 0,
  };

  auto magic = buffer.subspan(0, 8);

  if(!std::ranges::equal(magic, VALID_MAGIC))
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

  u32 version     = read_u32(buffer, cursor);
  u32 chunk_count = read_u32(buffer, cursor);
  spdlog::info("version: {}, chunk_count: {}", version, chunk_count);

  if(chunk_count < 2)
    WIND_ERR(WindError::internal());

  constexpr u32 CHUNK_VERT = 0x54524556;  // "VERT"
  constexpr u32 CHUNK_INDC = 0x43444E49;  // "INDC"
  constexpr u32 CHUNK_TEXT = 0x54584554;  // "TEXT"
  constexpr u32 CHUNK_MATE = 0x4554414D;  // "MATE"
  constexpr u32 CHUNK_END  = 0x4554414D;  // "IEND"

  // at this point we have read the header now comes the data


  spdlog::info("cursor before loop: {}", cursor);

  for(u32 i = 0; i < chunk_count; i++)
  {
    auto type = read_u32(buffer, cursor);
    spdlog::info("cursor after reading type: {}", cursor);

    switch(type)
    {
      case CHUNK_VERT: {
        auto size = read_u64(buffer, cursor);
        spdlog::info("vertices size: {} cursor: {}", size, cursor);
        // advance cursor
        cursor += size;
        spdlog::info("cursor after advancing vertices size: {}", cursor);
        break;
      }

      case CHUNK_INDC: {
        spdlog::info("cursor before reading indices size: {}", cursor);
        auto size = read_u64(buffer, cursor);
        spdlog::info("cursor after reading indices size: {}", cursor);
        spdlog::info("indices size: {}, cursor: {}", size, cursor);
        cursor += size;
        break;
      }

      case CHUNK_TEXT: {
        auto size   = read_u64(buffer, cursor);
        auto width  = read_u32(buffer, cursor);
        auto height = read_u32(buffer, cursor);
        spdlog::info("texture size: {}, cursor: {}", size, cursor);
        cursor += size - 8;
        break;
      }

        // case CHUNK_MATE: {
        //   auto size = read_u64(buffer, cursor);
        //   spdlog::info("material size: {}, cursor", size, cursor);
        //   cursor += size;
        //   break;
        // }

      default:
        break;
    }
  }

  return {};
}

WIND_NODISCARD WIND_INLINE auto load_asset(std::string_view name) WIND_NOEXCEPT -> WindResult<WindAsset>
{
  auto path = std::filesystem::path(name);

  if(!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path))
  {
    WIND_ERR(WindError::internal(ErrorCode::ResourceNotFound));
  }

  auto file_size = std::filesystem::file_size(path);

  if(file_size == 0)
  {
    WIND_ERR(WindError::internal());
  }

  std::ifstream file_stream(path, std::ios::binary);

  if(!file_stream.is_open())
  {
    WIND_ERR(WindError::internal());
  }

  std::vector<u8> buffer(file_size);

  if(!file_stream.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(file_size)))
  {
    WIND_ERR(WindError::internal());
  }

  spdlog::info("read {} bytes", buffer.size());

  auto data = WIND_TRY(decode(buffer));

  return {};
};


};  // namespace wind::asset
