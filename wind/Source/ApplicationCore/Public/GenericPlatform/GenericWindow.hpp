#pragma once

// =======================================================
// Generic window class that multiple platforms can inherit
// =======================================================

#include <cstdint>

struct FGenericWindowParams
{
	uint32_t Width;
	uint32_t Height;
	const char *Title;
};

class FGenericWindow
{
  public:
	FGenericWindow();

	virtual ~FGenericWindow();

	virtual void Destroy();
	virtual void Show();
	virtual void Hide();
	virtual void *GetOSWindowHandle();
};
