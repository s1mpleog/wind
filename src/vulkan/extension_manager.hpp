#pragma once

// handle the extensions for vulkan platform, instance, device...

#include "types.hpp"
#include <algorithm>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace wind::vulkan {
// :(
static constexpr usize INITIAL_EXTENSIONS_COUNT{10};

struct ExtensionManager
{
public:
  explicit ExtensionManager() { m_extensions.reserve(INITIAL_EXTENSIONS_COUNT); }

  // delete copy constructor and operator
  ExtensionManager(const ExtensionManager&)                   = delete;
  auto operator=(const ExtensionManager&) -> ExtensionManager = delete;

  auto push(const char* extension) -> void { m_extensions.emplace_back(extension); }
  auto push(std::span<const char*> extensions) noexcept -> void { m_extensions.append_range(extensions); };

  [[nodiscard]] auto get_extensions() noexcept -> std::span<const char*>
  {
    return std::span{m_extensions.data(), m_extensions.size()};
  }

  [[nodiscard]] auto find(std::string_view extension) const noexcept -> std::optional<std::string_view>
  {
    auto it = std::ranges::find(m_extensions, extension);

    if(it != m_extensions.end())
    {
      return std::string_view{*it};
    }

    return std::nullopt;
  }

private:
  std::vector<const char*> m_extensions;
};

}  // namespace wind::vulkan
