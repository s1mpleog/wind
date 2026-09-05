#include "Engine.hpp"

#include "ApplicationCore/Public/GenericPlatform/GenericWindow.hpp"
#include "Check.hpp"
#include "Vulkan/Core/Private/VulkanGenericPlatform.h"
#include "Vulkan/Core/Private/VulkanSwapchain.hpp"
#include "Vulkan/Core/Private/VulkanSynchronization.hpp"

// #include "Check.hpp"
// #include "Core/ServiceLocator.hpp"
// #include "Error.hpp"
// #include "Input/InputManager.hpp"
// #include "Platform/Window.hpp"
// #include "Resources/Builtin.hpp"
// #include "Resources/ResourceManager.hpp"
// #include "SDL3/SDL_events.h"
// #include "SDL3/SDL_video.h"
// #include "Scene/RenderObject.hpp"
// #include "Scene/Scene.hpp"
// #include "Utils/ExpectedUtil.hpp"
// #include "Vulkan/Core/Context.hpp"
// #include "Vulkan/Core/Private/VulkanCore.hpp"
// #include "Vulkan/Core/Private/VulkanGenericPlatform.h"
// #include "Vulkan/Core/Private/VulkanSwapchain.hpp"
// #include "Vulkan/Core/Swapchain.hpp"
// #include "Vulkan/Core/VulkanDevice.hpp"
// #include "Vulkan/Graphics/PipelineManager.hpp"
// #include "Vulkan/Renderer.hpp"
// #include "vulkan/vulkan.hpp"

#include <SDL3/SDL_timer.h>
#include <X11/X.h>
#include <spdlog/spdlog.h>

FEngine::FEngine(FConfiguration VulkanConfig, FGenericWindowParams InWindowParams)
    : Window(InWindowParams), Core(VulkanConfig)
{
}

void FEngine::Initialize()
{
	Window.Initialize();
	Core.Initialize();

	FVulkanSwapChain Swapchain{Core};

	uint32 DesiredImageCount = 3;

	FVulkanGenericPlatformWindowContext Context(Window.GetOSWindowHandle());

	Swapchain.Create(Context, 400, 600, &DesiredImageCount, nullptr);

	FVulkanFence Fence{*Core.GetDevice()};

	Fence.Create(true);

	FVulkanSemaphore Semaphore{*Core.GetDevice()};

	WIND_LOG(info, "Created semaphore {}", (void *)Semaphore.GetHandle());

	Fence.Destroy();

	Semaphore.Destroy();

	Swapchain.Destroy(nullptr);
}

void FEngine::Run() WIND_NOEXCEPT
{
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
