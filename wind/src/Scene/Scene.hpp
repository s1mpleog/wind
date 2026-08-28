#pragma once

// scene will own all the render_object and camera
// but someone have to push all the handles into scene

#include "Camera.hpp"
#include "Scene/RenderObject.hpp"

#include <vector>

class UScene
{
  public:
	auto AddRenderObjects(FBuiltAssets Assets, bool IsModel = true, FTransform Transform = {}) WIND_NOEXCEPT -> void;

	auto Get() WIND_NOEXCEPT -> std::vector<FRenderObject>;

	UScene()
	{
		Camera = FCamera{.Position = {0.0F, 1.75F, 7.5F},
		                 // .Position = {0.0f, 0.0f, 3.0f},
		                 .Yaw = glm::radians(180.0F),
		                 .Pitch = glm::radians(-9.0F),
		                 // .pitch = glm::radians(0.0f),
		                 .Fov = 60.0F,
		                 .Aspect = 1200.0F / 700.0F,
		                 .Near = 0.1F,
		                 .Far = 1000.0F};
	}

	// make camera public for now it does not have any dependency to hide
	FCamera Camera;

  private:
	std::vector<FRenderObject> MObjects;
};
