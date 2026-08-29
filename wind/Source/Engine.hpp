#pragma once

#include "Config.hpp"
#include "Input/InputManager.hpp"
#include "Platform/Window.hpp"
#include "Resources/ResourceManager.hpp"
#include "Scene/Scene.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Configuration.hpp"
#include "Vulkan/Core/Context.hpp"
#include "Vulkan/Graphics/PipelineManager.hpp"
#include "Vulkan/Renderer.hpp"

#include <memory>

class FUEngine
{
  public:
	FUEngine(const FUEngine &) = delete;
	auto operator=(const FUEngine &) -> FUEngine & = delete;

	FUEngine(FUEngine &&) noexcept = default;
	auto operator=(FUEngine &&) noexcept -> FUEngine & = default;

	WIND_NODISCARD static auto Create(FWindowConfiguration WindowCfg, FConfiguration VulkanCfg) WIND_NOEXCEPT
	    -> TWindResult<FUEngine>;

	auto Run() WIND_NOEXCEPT -> TWindResult<void>;

  private:
	FUEngine(FUWindow Window, std::unique_ptr<FVulkanContext> Context, FRenderer Renderer,
	         std::unique_ptr<FUInputManger> InputManager, std::unique_ptr<FUResourceManager> ResourceManager,
	         std::unique_ptr<FUPipelineManager> PipelineManager, FUScene Scene)
	    : MWindow{std::move(Window)}, MVulkanContext{std::move(Context)}, MRenderer{std::move(Renderer)},
	      MPipelineManager{std::move(PipelineManager)}, MResourceManager{std::move(ResourceManager)},
	      MInputManager{std::move(InputManager)}, MScene{std::move(Scene)} {};

	FUWindow MWindow;
	std::unique_ptr<FVulkanContext> MVulkanContext;
	FRenderer MRenderer;
	std::unique_ptr<FUPipelineManager> MPipelineManager;
	std::unique_ptr<FUResourceManager> MResourceManager;
	std::unique_ptr<FUInputManger> MInputManager;
	FUScene MScene;
};
