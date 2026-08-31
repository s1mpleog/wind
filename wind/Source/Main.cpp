#include "Engine.hpp"
#include "Platform/Window.hpp"
#include "Types.hpp"
#include "Vulkan/Core/Configuration.hpp"

#include <cstdlib>
#include <spdlog/spdlog.h>

auto main() -> int32
{
	constexpr uint16 WindowWidth = 1280;
	constexpr uint16 WindowHeight = 720;

	auto WindowCfg = FWindowConfiguration{.Name = "Wind", .Width = WindowWidth, .Height = WindowHeight};
	auto VulkanCfg = Default;
	VulkanCfg.AppName = "Wind";

	FEngine WindEngine = {std::move(VulkanCfg)};

	// if (!Engine)
	// {
	// 	spdlog::error("{}", Engine.error().to_string());
	// 	return EXIT_FAILURE;
	// }

	WindEngine.Run();

	return EXIT_SUCCESS;
}
