#include "ApplicationCore/Public/GenericPlatform/GenericWindow.hpp"
#include "Engine.hpp"
#include "Vulkan/Public/Configuration.hpp"

#include <cstdlib>
#include <spdlog/spdlog.h>

int32_t main()
{
	auto VulkanCfg = Default;

	FGenericWindowParams WindowParams{.Width = 1280, .Height = 720, .Title = "Wind"};

	FEngine WindEngine = {std::move(VulkanCfg), WindowParams};

	WindEngine.Initialize();

	WindEngine.Run();

	WindEngine.Destroy();

	return EXIT_SUCCESS;
}
