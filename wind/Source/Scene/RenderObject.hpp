#pragma once

#include "Resources/Builtin.hpp"
#include "Resources/ResourceManager.hpp"
#include "Vulkan/Graphics/PipelineManager.hpp"

struct FModelComponent
{
	TModelHandle Handle;
};

struct FPipelineComponent
{
	TPipelineHandle Handle;
};

struct FTransform
{
};

struct FRenderObject
{
	TModelHandle ModelHandle{};
	TPipelineHandle PipelineHandle{};
	bool IsModelType{};
	FBufferAssets BufferAsset{};
	// todo: transform later
};
