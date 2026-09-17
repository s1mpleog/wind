#pragma once

#include "Configuration.hpp"
#include "GenericPlatform/GenericWindow.hpp"
#include "LinuxWindow.hpp"
#include "Renderer.hpp"
#include "VulkanContext.hpp"

class FEngine
{
  public:
	FEngine(const FEngine &) = delete;
	auto operator=(const FEngine &) -> FEngine & = delete;

	FEngine(FEngine &&) noexcept = default;
	auto operator=(FEngine &&) noexcept -> FEngine & = default;

	FEngine(FConfiguration VulkanConfig, FGenericWindowParams InWindowParams);

	void Initialize();

	void Run() noexcept;

	void Destroy();

  private:
	// TODO: for now just use core type later add unique ptr and use generic platform instead
	FLinuxWindow Window;
	FVulkanContext Context;
	std::unique_ptr<FVulkanRenderer> Renderer;
};
