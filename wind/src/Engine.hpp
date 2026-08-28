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

class UEngine
{
  public:
	UEngine(const UEngine &) = delete;
	auto operator=(const UEngine &) -> UEngine & = delete;

	UEngine(UEngine &&) noexcept = default;
	auto operator=(UEngine &&) noexcept -> UEngine & = default;

	WIND_NODISCARD static auto Create(FWindowConfiguration WindowCfg, FConfiguration VulkanCfg) WIND_NOEXCEPT
	    -> TWindResult<UEngine>;

	auto Run() WIND_NOEXCEPT -> TWindResult<void>;

  private:
	UEngine(UWindow Window, std::unique_ptr<FVulkanContext> Context, URenderer Renderer,
	        std::unique_ptr<UInputManger> InputManager, std::unique_ptr<UResourceManager> ResourceManager,
	        std::unique_ptr<UPipelineManager> PipelineManager, UScene Scene)
	    : MWindow{std::move(Window)}, MVulkanContext{std::move(Context)}, MRenderer{std::move(Renderer)},
	      MPipelineManager{std::move(PipelineManager)}, MResourceManager{std::move(ResourceManager)},
	      MInputManager{std::move(InputManager)}, MScene{std::move(Scene)} {};

	UWindow MWindow;
	std::unique_ptr<FVulkanContext> MVulkanContext;
	URenderer MRenderer;
	std::unique_ptr<UPipelineManager> MPipelineManager;
	std::unique_ptr<UResourceManager> MResourceManager;
	std::unique_ptr<UInputManger> MInputManager;
	UScene MScene;
};
