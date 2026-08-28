#include "Scene/Scene.hpp"

#include "Resources/Builtin.hpp"
#include "Resources/ResourceManager.hpp"
#include "Scene/RenderObject.hpp"

auto UScene::AddRenderObjects(FBuiltAssets Assets, bool IsModel, FTransform Transform) WIND_NOEXCEPT -> void
{
	auto RenderObject = FRenderObject{.ModelHandle = IsModel ? Assets.Models : TModelHandle{},
	                                  .PipelineHandle = Assets.Pipelines,
	                                  .IsModelType = IsModel,
	                                  .BufferAsset = IsModel ? FBufferAssets{} : Assets.BufferAssets};

	MObjects.push_back(RenderObject);
}

auto UScene::Get() WIND_NOEXCEPT -> std::vector<FRenderObject>
{
	return MObjects;
}
