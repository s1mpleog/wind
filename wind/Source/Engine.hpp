#pragma once

#include "ApplicationCore/Public/GenericPlatform/GenericWindow.hpp"
#include "ApplicationCore/Public/LinuxWindow.hpp"
#include "Config.hpp"
#include "Renderer.hpp"
#include "Vulkan/Public/Configuration.hpp"
#include "Vulkan/Public/VulkanContext.hpp"

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
	FVulkanContext Context;
	std::unique_ptr<FVulkanRenderer> Renderer;
};
