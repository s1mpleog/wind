#pragma once

#include "Types.hpp"

#include <string_view>
#include <vulkan/vulkan_core.h>

enum class EVulkanVersion : u8
{
	VERSION13,
	VERSION14,
};

enum class EBuffering : u8
{
	DoubleBuffering = 2,
	TripleBuffering = 3,
};

enum class EDebugMessageSeverity : u16
{
	Verbose = 0x00000001,
	Info = 0x00000010,
	Warning = 0x00000100,
	Error = 0x00001000,
};

enum class EDebugMessageType : u8
{
	General = 1 << 0,
	Validation = 1 << 1,
	Performance = 1 << 2,
	DeviceAddressBinding = 1 << 3
};

constexpr auto operator|(EDebugMessageSeverity Lhs, EDebugMessageSeverity Rhs) WIND_NOEXCEPT->EDebugMessageSeverity
{
	using TU = std::underlying_type_t<EDebugMessageSeverity>;

	return static_cast<EDebugMessageSeverity>(static_cast<TU>(Lhs) | static_cast<TU>(Rhs));
}

constexpr auto operator|(EDebugMessageType Lhs, EDebugMessageType Rhs) WIND_NOEXCEPT->EDebugMessageType
{
	using TU = std::underlying_type_t<EDebugMessageType>;

	return static_cast<EDebugMessageType>(static_cast<TU>(Lhs) | static_cast<TU>(Rhs));
}

struct FConfiguration
{
	EVulkanVersion ApiVersion{EVulkanVersion::VERSION14};
	bool Vsync{true};
	EBuffering Buffering{EBuffering::DoubleBuffering};
	std::string_view AppName;
	std::string_view EngineName{"NoEngine"};
	EDebugMessageSeverity DebugMessageSeverity{EDebugMessageSeverity::Error};
	EDebugMessageType DebugMessageType{EDebugMessageType::General};
};

WIND_INLINE constexpr FConfiguration Default{
    .ApiVersion = EVulkanVersion::VERSION14,
    .Vsync = true,
    .Buffering = EBuffering::DoubleBuffering,
    .AppName = "application",
    .EngineName = "NoEngine",
    .DebugMessageSeverity = EDebugMessageSeverity::Error,
    .DebugMessageType = EDebugMessageType::General,
};

WIND_INLINE constexpr FConfiguration Development{
    .ApiVersion = EVulkanVersion::VERSION14,
    .Vsync = false,
    .Buffering = EBuffering::DoubleBuffering,
    .AppName = "application",
    .EngineName = "NoEngine",
    .DebugMessageSeverity =
        EDebugMessageSeverity::Error | EDebugMessageSeverity::Warning | EDebugMessageSeverity::Verbose,
    .DebugMessageType = EDebugMessageType::General | EDebugMessageType::Performance | EDebugMessageType::Validation,
};
