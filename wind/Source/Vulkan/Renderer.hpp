#pragma once

#include "Config.hpp"
#include "RenderView.hpp"
#include "Resources/ResourceManager.hpp"
#include "Scene/RenderObject.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Configuration.hpp"
#include "Vulkan/Core/Context.hpp"
#include "Vulkan/Core/Swapchain.hpp"
#include "Vulkan/Frame/FrameContext.hpp"
#include "Vulkan/Graphics/PipelineManager.hpp"

#include <vulkan/vulkan_raii.hpp>

class FRenderer
{
  public:
	FRenderer(const FRenderer &) = delete;
	auto operator=(const FRenderer &) -> FRenderer & = delete;
	FRenderer(FRenderer &&) noexcept = default;
	auto operator=(FRenderer &&) noexcept -> FRenderer & = default;

	// two step initialization
	// todo: later make resource and pipeline manager const
	WIND_NODISCARD static auto Create(FConfiguration Cfg, const FUWindow &Window, const FVulkanContext *Context,
	                                  FUResourceManager *ResourceManager,
	                                  FUPipelineManager *PipelineManager) WIND_NOEXCEPT -> TWindResult<FRenderer>;

	WIND_NODISCARD auto Shutdown() const WIND_NOEXCEPT -> TWindResult<void>
	{
		WIND_TRY(Context->GpuDevice.Device.waitIdle());
		return {};
	}

	WIND_NODISCARD auto Begin(uint32 Width, uint32 Height) WIND_NOEXCEPT -> TWindResult<void>;
	auto Draw(FRenderObject Object, FRenderView CameraView) WIND_NOEXCEPT -> void;
	auto End() WIND_NOEXCEPT -> void;

	// internal functions
	auto DrawModel(FRenderObject Object, FRenderView CameraView, vk::raii::CommandBuffer &CmdBuffer) WIND_NOEXCEPT
	    -> void;
	auto DrawBuffer(FRenderObject Object, FRenderView CameraView, vk::raii::CommandBuffer &CmdBuffer) WIND_NOEXCEPT
	    -> void;

	// can cmd buffer be const ?
	auto SetupViewport(vk::raii::CommandBuffer &CmdBuffer) const WIND_NOEXCEPT -> void;

  private:
	FRenderer(FConfiguration Cfg, const FVulkanContext *Context, FSwapchainContext SwapchainContext,
	          std::vector<FFRameContext> FrameContext, FUResourceManager *ResourceManager,
	          FUPipelineManager *PipelineManager, TDynamicBufferHandle FrameUbo)
	    : Config{Cfg}, Context{Context}, SwapchainContext{std::move(SwapchainContext)},
	      FrameContext(std::move(FrameContext)), PipelineManager{PipelineManager}, ResourceManager{ResourceManager},
	      FrameUbo{FrameUbo} {};

	FConfiguration Config;
	const FVulkanContext *Context;
	FSwapchainContext SwapchainContext;
	std::vector<FFRameContext> FrameContext;
	FUPipelineManager *PipelineManager;
	FUResourceManager *ResourceManager;
	TDynamicBufferHandle FrameUbo{};

	uint32 CurrentFrame{0};
	uint32 CurrentImage{0};
};
