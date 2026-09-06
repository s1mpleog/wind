#pragma once

#include <cstdint>
#include <string_view>
#include <vulkan/vulkan_core.h>

enum class EVulkanVersion : uint8_t
{
	VERSION13,
	VERSION14,
};

enum class EBuffering : uint8_t
{
	DoubleBuffering = 2,
	TripleBuffering = 3,
};

enum class EDebugMessageSeverity : uint16_t
{
	Verbose = 0x00000001,
	Info = 0x00000010,
	Warning = 0x00000100,
	Error = 0x00001000,
};

enum class EDebugMessageType : uint8_t
{
	General = 1 << 0,
	Validation = 1 << 1,
	Performance = 1 << 2,
	DeviceAddressBinding = 1 << 3
};

constexpr auto operator|(EDebugMessageSeverity Lhs, EDebugMessageSeverity Rhs) noexcept -> EDebugMessageSeverity
{
	using TU = std::underlying_type_t<EDebugMessageSeverity>;

	return static_cast<EDebugMessageSeverity>(static_cast<TU>(Lhs) | static_cast<TU>(Rhs));
}

constexpr auto operator|(EDebugMessageType Lhs, EDebugMessageType Rhs) noexcept -> EDebugMessageType
{
	using TU = std::underlying_type_t<EDebugMessageType>;

	return static_cast<EDebugMessageType>(static_cast<TU>(Lhs) | static_cast<TU>(Rhs));
}

struct FConfiguration
{
	EVulkanVersion ApiVersion{EVulkanVersion::VERSION14};
	bool Vsync{true};
	EBuffering Buffering{EBuffering::DoubleBuffering};
	std::string_view AppName{"Wind"};
	std::string_view EngineName{"NoEngine"};
	EDebugMessageSeverity DebugMessageSeverity{EDebugMessageSeverity::Error};
	EDebugMessageType DebugMessageType{EDebugMessageType::General};
};

inline constexpr FConfiguration Default{
    .ApiVersion = EVulkanVersion::VERSION14,
    .Vsync = true,
    .Buffering = EBuffering::DoubleBuffering,
    .AppName = "application",
    .EngineName = "NoEngine",
    .DebugMessageSeverity = EDebugMessageSeverity::Error,
    .DebugMessageType = EDebugMessageType::General,
};

inline constexpr FConfiguration Development{
    .ApiVersion = EVulkanVersion::VERSION14,
    .Vsync = false,
    .Buffering = EBuffering::DoubleBuffering,
    .AppName = "application",
    .EngineName = "NoEngine",
    .DebugMessageSeverity =
        EDebugMessageSeverity::Error | EDebugMessageSeverity::Warning | EDebugMessageSeverity::Verbose,
    .DebugMessageType = EDebugMessageType::General | EDebugMessageType::Performance | EDebugMessageType::Validation,
};
