#include "ApplicationCore/Public/GenericPlatform/GenericWindow.hpp"
#include "Engine.hpp"
#include "Types.hpp"
#include "Vulkan/Core/Configuration.hpp"

#include <cstdlib>
#include <spdlog/spdlog.h>

auto main() -> int32
{
	auto VulkanCfg = Default;

	FGenericWindowParams WindowParams{.Width = 1280, .Height = 720, .Title = "Wind"};

	FEngine WindEngine = {std::move(VulkanCfg), WindowParams};

	WindEngine.Initialize();

	WindEngine.Run();

	WindEngine.Destroy();

	return EXIT_SUCCESS;
}
