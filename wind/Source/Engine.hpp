#pragma once

#include "ApplicationCore/Public/GenericPlatform/GenericWindow.hpp"
#include "ApplicationCore/Public/LinuxWindow.hpp"
#include "Config.hpp"
// #include "Input/InputManager.hpp"
// #include "Resources/ResourceManager.hpp"
// #include "Scene/Scene.hpp"
#include "Vulkan/Core/Configuration.hpp"
// #include "Vulkan/Core/Context.hpp"
#include "Vulkan/Core/Private/VulkanCore.hpp"
// #include "Vulkan/Graphics/PipelineManager.hpp"
// #include "Vulkan/Renderer.hpp"

class FEngine
{
  public:
	FEngine(const FEngine &) = delete;
	auto operator=(const FEngine &) -> FEngine & = delete;

	FEngine(FEngine &&) noexcept = default;
	auto operator=(FEngine &&) noexcept -> FEngine & = default;

	FEngine(FConfiguration VulkanConfig, FGenericWindowParams InWindowParams);

	void Initialize();

	void Run() WIND_NOEXCEPT;

	void Destroy();

  private:
	// TODO: for now just use core type later add unique ptr and use generic platform instead
	FLinuxWindow Window;
	FVulkanCore Core;
	// std::unique_ptr<FVulkanContext> MVulkanContext;
	// FRenderer MRenderer;
	// std::unique_ptr<FUPipelineManager> MPipelineManager;
	// std::unique_ptr<FUResourceManager> MResourceManager;
	// std::unique_ptr<FUInputManger> MInputManager;
	// FUScene MScene;
};
