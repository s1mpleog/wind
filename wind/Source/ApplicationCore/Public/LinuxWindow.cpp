#include "LinuxWindow.hpp"

#include "Check.hpp"

#include <SDL3/SDL_video.h>

FLinuxWindow::FLinuxWindow(FGenericWindowParams InWindowParams) : HWnd(nullptr), WindowParams(std::move(InWindowParams))
{
	if (!bIsSdlInit)
	{
		if (SDL_Init(SDL_INIT_VIDEO) == false)
		{
			FATAL("Failed to initialize SDL3 Error code: {}", SDL_GetError());
		}

		bIsSdlInit = true;
	}
}

FLinuxWindow::~FLinuxWindow()
{
}

void FLinuxWindow::Initialize()
{
	CHECK(bIsSdlInit);
	HWnd = SDL_CreateWindow(WindowParams.Title, WindowParams.Width, WindowParams.Height, SDL_WINDOW_VULKAN);

	if (HWnd == nullptr)
	{
		FATAL("Failed to create SDL window Error code: {}", SDL_GetError());
	}

	WIND_LOG(info, "[SDL3] Window handle created successfully");
}

void FLinuxWindow::Destroy()
{
	if (HWnd != nullptr)
	{
		SDL_DestroyWindow(HWnd);
	}
}
