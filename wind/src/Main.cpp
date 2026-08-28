#include "Engine.hpp"
#include "Platform/Window.hpp"
#include "Types.hpp"
#include "Vulkan/Core/Configuration.hpp"

#include <cstdlib>
#include <spdlog/spdlog.h>

auto main() -> TI32
{
	constexpr TU16 WindowWidth = 1280;
	constexpr TU16 WindowHeight = 720;

	auto WindowCfg = FWindowConfiguration{.Name = "Wind", .Width = WindowWidth, .Height = WindowHeight};
	auto VulkanCfg = Default;
	VulkanCfg.AppName = "Wind";

	auto Engine = FUEngine::Create(std::move(WindowCfg), VulkanCfg);

	if (!Engine)
	{
		spdlog::error("{}", Engine.error().to_string());
		return EXIT_FAILURE;
	}

	if (!Engine->Run())
	{
		spdlog::error("{}", Engine.error().to_string());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
