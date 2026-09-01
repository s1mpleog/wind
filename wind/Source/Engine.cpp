#include "Engine.hpp"

#include "Check.hpp"
#include "Core/ServiceLocator.hpp"
#include "Error.hpp"
#include "Input/InputManager.hpp"
#include "Platform/Window.hpp"
#include "Resources/Builtin.hpp"
#include "Resources/ResourceManager.hpp"
#include "SDL3/SDL_events.h"
#include "Scene/RenderObject.hpp"
#include "Scene/Scene.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Context.hpp"
#include "Vulkan/Core/Private/VulkanCore.hpp"
#include "Vulkan/Core/Private/VulkanGenericPlatform.h"
#include "Vulkan/Core/Private/VulkanSwapchain.hpp"
#include "Vulkan/Core/Swapchain.hpp"
#include "Vulkan/Core/VulkanDevice.hpp"
#include "Vulkan/Graphics/PipelineManager.hpp"
#include "Vulkan/Renderer.hpp"
#include "vulkan/vulkan.hpp"

#include <SDL3/SDL_timer.h>
#include <X11/X.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <vector>

// WIND_NODISCARD auto FUEngine::Create(FWindowConfiguration WindowCfg, FConfiguration VulkanCfg) WIND_NOEXCEPT
//     -> TWindResult<FUEngine>
// {
// #ifdef WIND_LOG_ENABLE
// 	spdlog::info("initializing Engine...");
// #endif

// 	auto Window = FUWindow{std::move(WindowCfg)};
// 	WIND_TRY_VOID(Window.Create());

// 	auto InputManager = std::make_unique<FUInputManger>(FUInputManger{});
// 	FUServiceLocator::Provide(InputManager.get());

// 	auto VulkanContext = std::make_unique<FVulkanContext>(WIND_TRY(CreateContext(Window, VulkanCfg)));

// 	auto ResourceManager =
// 	    std::make_unique<FUResourceManager>(WIND_TRY(FUResourceManager::Create(VulkanContext.get())));

// 	auto PipelineManager = std::make_unique<FUPipelineManager>(FUPipelineManager{});

// 	// load all the models and pipelines
// 	auto Assets = WIND_TRY(Build(ResourceManager.get(), PipelineManager.get(), VulkanContext->GpuDevice.Device));

// 	FUScene Scene{};

// 	for (const auto &Asset : Assets)
// 	{
// 		Scene.AddRenderObjects(Asset, Asset.IsModel);
// 	}

// 	auto Renderer = WIND_TRY(
// 	    FRenderer::Create(VulkanCfg, Window, VulkanContext.get(), ResourceManager.get(), PipelineManager.get()));

// #ifdef WIND_LOG_ENABLE
// 	spdlog::info("Engine created successfully");
// #endif

// 	return FUEngine(std::move(Window), std::move(VulkanContext), std::move(Renderer), std::move(InputManager),
// 	                std::move(ResourceManager), std::move(PipelineManager), std::move(Scene));
// }

FEngine::FEngine(FConfiguration VulkanConfig) : Core(VulkanConfig)
{
	auto cfg = FWindowConfiguration{.Name = "test", .Width = 400, .Height = 200};

	auto Window = FUWindow{cfg};
	if (!Window.Create())
	{
		WIND_LOG(info, "Failed to create window");
		return;
	}

	Core.Initialize();

	FVulkanGenericPlatformWindowContext Context(Window.Handle());

	uint32 MinImageCount = 3;
	std::vector<vk::Image> Images;

	auto swapchain = FVulkanSwapChain::Create(Core.GetInstance(), *Core.GetDevice(), 200, 400, &MinImageCount, Images,
	                                          Context, nullptr);
}

auto FEngine::Run() WIND_NOEXCEPT -> void
{
	return;
}

// auto FUEngine::Run() WIND_NOEXCEPT -> TWindResult<void>
// {
// 	bool Running = true;

// 	[[maybe_unused]] float Time = 0.0F;

// 	uint64 Last = SDL_GetPerformanceCounter();

// 	// TODO: abstract this
// 	while (Running)
// 	{
// 		uint64_t Now = SDL_GetPerformanceCounter();
// 		float Delta = static_cast<float>(Now - Last) / static_cast<float>(SDL_GetPerformanceFrequency());
// 		Last = Now;
// 		Time += Delta;

// 		SDL_Event Event{};

// 		MInputManager->BeginFrame();

// 		while (SDL_PollEvent(&Event))
// 		{
// 			MInputManager->ProcessEvent(Event);

// 			if (Event.type == SDL_EVENT_WINDOW_RESIZED)
// 			{
// 				int NewWidth = Event.window.data1;
// 				int NewHeight = Event.window.data2;
// 				// Handle new dimensions here
// 				break;
// 			}

// 			if (Event.type == SDL_EVENT_QUIT)
// 				Running = false;
// 		}

// 		// TODO: do not use this
// 		// m_input_manager->update();

// 		int Width{};
// 		int Height{};

// 		SDL_GetWindowSizeInPixels(MWindow.Handle(), &Width, &Height);

// 		MScene.Camera.UpdateAspect(Width, Height);

// 		MScene.Camera.ProcessMouse();
// 		MScene.Camera.ProcessKeyboard(Delta);

// 		auto BeginResult = MRenderer.Begin(static_cast<uint32>(Width), static_cast<uint32>(Height));

// 		if (!BeginResult)
// 		{
// 			if (BeginResult.error().code == ErrorCode::SwapchainOutOfDate)
// 				continue;

// 			WIND_ERR(BeginResult.error());
// 		}

// 		auto CameraView = MScene.Camera.RenderView();

// 		for (const auto &Object : MScene.Get())
// 		{
// 			MRenderer.Draw(Object, CameraView);
// 		}

// 		MRenderer.End();
// 	}

// 	WIND_TRY(MRenderer.Shutdown());

// 	return {};
// }
