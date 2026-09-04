#pragma once

// =======================================================
// Generic window class that multiple platforms can inherit
// =======================================================

#include "Types.hpp"

struct FGenericWindowParams
{
	uint32 Width;
	uint32 Height;
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
