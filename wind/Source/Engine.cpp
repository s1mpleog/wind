#include "Engine.hpp"

#include "ApplicationCore/Public/GenericPlatform/GenericWindow.hpp"
#include "Check.hpp"
#include "Renderer/Public/FrameContext.hpp"
#include "Renderer/Public/Renderer.hpp"
#include "Vulkan/Public/VulkanContext.hpp"
#include "VulkanGenericPlatform.h"

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

	// create renderer once context is valid

	// create a presentation target for renderer
	FPresentationTarget Target{.WindowContext = FVulkanGenericPlatformWindowContext(Window.GetOSWindowHandle()),
	                           .Width = Window.GetWidth(),
	                           .Height = Window.GetHeight()};

	Renderer = std::make_unique<FVulkanRenderer>(&Context, Target);
	CHECK(Renderer);

	Renderer->Initialize();
}

void FEngine::Run() WIND_NOEXCEPT
{
	bool bIsRunning = true;

	while (bIsRunning)
	{
		SDL_Event Event{};

		while (SDL_PollEvent(&Event))
		{
			if (Event.type == SDL_EVENT_QUIT)
			{
				bIsRunning = false;
			}

			if (Event.type == SDL_EVENT_WINDOW_RESIZED)
			{
				// todo: add a event bus record change dimesion here
				// and begin should automatically get new dimensions
			}
		}

		int Width{};
		int Height{};

		SDL_GetWindowSizeInPixels(static_cast<SDL_Window *>(Window.GetOSWindowHandle()), &Width, &Height);

		FFrameResult BeginResult = Renderer->BeginFrame(Width, Height);

		if (!BeginResult)
		{
			switch (BeginResult.error())
			{
			case EFrameError::Fatal:
				FATAL("Something really went wrong engine can not continue");

			case EFrameError::OutOfDate:
				continue;

			case EFrameError::DeviceLost:
				FATAL("Vulkan lost the communication with GPU terminating");

			case EFrameError::SubOptimal:
				continue;

			default:
				std::unreachable();
			}
		}

		Renderer->Draw();

		FFrameResult EndResult = Renderer->EndFrame();

		if (!EndResult)
		{
			switch (EndResult.error())
			{
			case EFrameError::Fatal:
				FATAL("Something really went wrong engine can not continue");

			case EFrameError::DeviceLost:
				FATAL("Vulkan lost the communication with GPU terminating");

			default:
				std::unreachable();
			}
		}
	}
}

void FEngine::Destroy()
{
	Window.Destroy();
}
