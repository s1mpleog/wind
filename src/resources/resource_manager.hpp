#pragma once

// i want it to be like this
// TextureHandle texture = resources<Texture>.load("assets/texture.png")
// ShaderHandle shader = resources<Shader>.load(device, "assets/shader.vert")
// where, Texture and Shader is a handle not underlying resource
// conceptually struct Handle { u32 index; u32 generation }
// provide options like get and delete do validation internally
// flow:
// TextureHandle tex = resource<Texture>.load("...");
// internally validate does file exists ? ...
// check the cache if hit -> increment index return handle
// load the file -> cache it -> increment handle data -> return handle


#include "error.hpp"
#include "utils/expected_util.hpp"
#include <filesystem>
#include <string_view>

namespace wind {

template <typename T>
struct Handle
{
  u32 index{};
  u32 generation{};
};

struct Texture;
struct Shader;

using TextureHandle = Handle<Texture>;
using ShaderHandle  = Handle<Shader>;

static_assert(sizeof(TextureHandle) == 8);
static_assert(sizeof(ShaderHandle) == 8);

static_assert(!std::is_same_v<TextureHandle, ShaderHandle>);

class ResourceManager
{
public:
  //TODO: constrain T
  template <typename T>
  WIND_NODISCARD auto load(std::string_view path) WIND_NOEXCEPT -> WindResult<Handle<T>>
  {
    if(!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path))
      WIND_ERR(WindError::internal(ErrorCode::ResourceNotFound));

    // do validation, cache check loading and all

    return Handle<T>{.index = 1, .generation = 0};
  }

  template <typename T>
  WIND_NODISCARD auto load() WIND_NOEXCEPT -> WindResult<T*>;

private:
};

};  // namespace wind

inline auto test() -> WindResult<void>
{
  wind::ResourceManager manager{};
  auto                  texture = WIND_TRY(manager.load<wind::TextureHandle>("src/main.cc"));

  return {};
}
