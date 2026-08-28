#pragma once

#include "./Types.hpp"
#include <vulkan/vulkan_core.h>
#include <string_view>

namespace wind::vulkan {
enum class VulkanVersion : u8
{
  VERSION13,
  VERSION14,
};

enum class Buffering : u8
{
  DoubleBuffering = 2,
  TripleBuffering = 3,
};

enum class DebugMessageSeverity : u16
{
  Verbose = 0x00000001,
  Info    = 0x00000010,
  Warning = 0x00000100,
  Error   = 0x00001000,
};

enum class DebugMessageType : u8
{
  General              = 1 << 0,
  Validation           = 1 << 1,
  Performance          = 1 << 2,
  DeviceAddressBinding = 1 << 3
};

constexpr auto operator|(DebugMessageSeverity lhs, DebugMessageSeverity rhs) WIND_NOEXCEPT->DebugMessageSeverity
{
  using U = std::underlying_type_t<DebugMessageSeverity>;

  return static_cast<DebugMessageSeverity>(static_cast<U>(lhs) | static_cast<U>(rhs));
}

constexpr auto operator|(DebugMessageType lhs, DebugMessageType rhs) WIND_NOEXCEPT->DebugMessageType
{
  using U = std::underlying_type_t<DebugMessageType>;

  return static_cast<DebugMessageType>(static_cast<U>(lhs) | static_cast<U>(rhs));
}

struct Configuration
{
  VulkanVersion        api_version{VulkanVersion::VERSION14};
  bool                 vsync{true};
  Buffering            buffering{Buffering::DoubleBuffering};
  std::string_view     app_name;
  std::string_view     engine_name{"NoEngine"};
  DebugMessageSeverity debug_message_severity{DebugMessageSeverity::Error};
  DebugMessageType     debug_message_type{DebugMessageType::General};
};


namespace presets {
WIND_INLINE constexpr Configuration Default{
    .api_version            = VulkanVersion::VERSION14,
    .vsync                  = true,
    .buffering              = Buffering::DoubleBuffering,
    .app_name               = "application",
    .engine_name            = "NoEngine",
    .debug_message_severity = DebugMessageSeverity::Error,
    .debug_message_type     = DebugMessageType::General,
};

WIND_INLINE constexpr Configuration Development{
    .api_version = VulkanVersion::VERSION14,
    .vsync       = false,
    .buffering   = Buffering::DoubleBuffering,
    .app_name    = "application",
    .engine_name = "NoEngine",
    .debug_message_severity = DebugMessageSeverity::Error | DebugMessageSeverity::Warning | DebugMessageSeverity::Verbose,
    .debug_message_type = DebugMessageType::General | DebugMessageType::Performance | DebugMessageType::Validation,
};

}  // namespace presets

};  // namespace wind::vulkan
