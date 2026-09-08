#include "Engine.hpp"

#include "ApplicationCore/Public/GenericPlatform/GenericWindow.hpp"
#include "Check.hpp"
#include "Renderer/Public/FrameContext.hpp"
#include "Vulkan/Private/VulkanGenericPlatform.h"
#include "Vulkan/Public/VulkanContext.hpp"

#include <SDL3/SDL_timer.h>
#include <spdlog/spdlog.h>

FEngine::FEngine(FConfiguration VulkanConfig, FGenericWindowParams InWindowParams)
    : Window(InWindowParams), Context(VulkanConfig)
{
}

void FEngine::Initialize()
{
	Window.Initialize();

	Context.Initialize();

	FVulkanGenericPlatformWindowContext WindowContext(Window.GetOSWindowHandle());
	uint32_t DesiredImageCount = 3;

	Context.CreateSwapchain(WindowContext, 1920, 1080, &DesiredImageCount);

	// do other setups

	// create renderer once context is valid
	Renderer = std::make_unique<FVulkanRenderer>(&Context);
	CHECK(Renderer);
}

void FEngine::Run() WIND_NOEXCEPT
{
	Renderer->BeginFrame();
	return;
}

void FEngine::Destroy()
{
	Window.Destroy();
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
