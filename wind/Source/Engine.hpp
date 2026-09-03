#pragma once

#include "Check.hpp"
#include "Config.hpp"
// #include "Input/InputManager.hpp"
#include "Platform/Window.hpp"
// #include "Resources/ResourceManager.hpp"
// #include "Scene/Scene.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Configuration.hpp"
// #include "Vulkan/Core/Context.hpp"
#include "Vulkan/Core/Private/VulkanCore.hpp"
// #include "Vulkan/Graphics/PipelineManager.hpp"
// #include "Vulkan/Renderer.hpp"

#include <memory>

class FEngine
{
  public:
	FEngine(const FEngine &) = delete;
	auto operator=(const FEngine &) -> FEngine & = delete;

	FEngine(FEngine &&) noexcept = default;
	auto operator=(FEngine &&) noexcept -> FEngine & = default;

	static FUWindow test()
	{
		auto cfg = FWindowConfiguration{.Name = "test", .Width = 400, .Height = 200};

		auto Window = FUWindow{cfg};
		if (!Window.Create())
		{
			WIND_LOG(info, "Failed to create window");
		}

		return Window;
	}

	FEngine(FConfiguration VulkanConfig, void *handle);

	WIND_NODISCARD static auto Create(FWindowConfiguration WindowCfg, FConfiguration VulkanCfg) WIND_NOEXCEPT
	    -> TWindResult<FEngine>;

	auto Run() WIND_NOEXCEPT -> void;

  private:
	//  FEngine(FUWindow Window, std::unique_ptr<FVulkanContext> Context, FRenderer Renderer,
	//          std::unique_ptr<FUInputManger> InputManager, std::unique_ptr<FUResourceManager> ResourceManager,
	//          std::unique_ptr<FUPipelineManager> PipelineManager, FUScene Scene)
	//     : MWindow{std::move(Window)}, MVulkanContext{std::move(Context)}, MRenderer{std::move(Renderer)},
	//       MPipelineManager{std::move(PipelineManager)}, MResourceManager{std::move(ResourceManager)},
	//       MInputManager{std::move(InputManager)}, MScene{std::move(Scene)} {};

	// FUWindow MWindow;
	FVulkanCore Core;
	// std::unique_ptr<FVulkanContext> MVulkanContext;
	// FRenderer MRenderer;
	// std::unique_ptr<FUPipelineManager> MPipelineManager;
	// std::unique_ptr<FUResourceManager> MResourceManager;
	// std::unique_ptr<FUInputManger> MInputManager;
	// FUScene MScene;
};
