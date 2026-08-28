#pragma once

#include "Core/ServiceLocator.hpp"
#include "Input/InputManager.hpp"
#include "RenderView.hpp"
#include "SDL3/SDL_scancode.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/trigonometric.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

struct FCamera
{
	glm::vec3 Position{0.0F, 1.75F, 7.5F};
	float Yaw{glm::radians(180.0F)};
	float Pitch{glm::radians(-9.0F)};

	float Fov = 60.0F;
	float Aspect = 16.0F / 9.0F;
	float Near = 0.1F;
	float Far = 1000.0F;
	bool IgnoreNextMouseMotion = true;

	WIND_NODISCARD auto Forward() const WIND_NOEXCEPT -> glm::vec3
	{
		return glm::normalize(
		    glm::vec3(glm::cos(Pitch) * glm::sin(Yaw), glm::sin(Pitch), glm::cos(Pitch) * glm::cos(Yaw)));
	}

	// auto right() const -> glm::vec3 { return glm::normalize(glm::cross(glm::vec3(0.0F, 1.0F, 0.0F), forward())); }

	[[nodiscard]] auto Right() const -> glm::vec3
	{
		return glm::normalize(glm::cross(Forward(), glm::vec3(0.0F, 1.0F, 0.0F)));
	}

	[[nodiscard]] auto View() const -> glm::mat4
	{
		return glm::lookAt(Position, Position + Forward(), glm::vec3(0.0F, 1.0F, 0.0F));
	}

	[[nodiscard]] auto SkyboxView() const -> glm::mat4
	{
		return glm::mat4(glm::mat3(View()));
	}

	WIND_NODISCARD auto Projection() const WIND_NOEXCEPT -> glm::mat4
	{
		auto Proj = glm::perspective(glm::radians(Fov), Aspect, Near, Far);
		Proj[1][1] *= -1.0F; // Vulkan Y-Flip
		return Proj;
	}

	[[nodiscard]] auto ViewProj() const -> glm::mat4
	{
		return Projection() * View();
	}

	auto UpdateAspect(TU32 Width, TU32 Height) WIND_NOEXCEPT -> void
	{
		Aspect = static_cast<float>(Width) / static_cast<float>(Height);
	}

	WIND_NODISCARD auto RenderView() const WIND_NOEXCEPT -> FRenderView
	{
		return FRenderView{
		    .View = View(),
		    .Projection = Projection(),
		};
	}

	auto ProcessKeyboard(float Delta) -> void
	{
		const float Speed = 10.0F * Delta;

		const glm::vec3 F = Forward();
		const glm::vec3 R = Right();
		const glm::vec3 U = glm::normalize(glm::cross(R, F)); // proper up

		auto &Input = UServiceLocator::Get<UInputManger>();

		if (Input.IsDown(SDL_SCANCODE_W))
			Position += F * Speed; // Forward

		if (Input.IsDown(SDL_SCANCODE_S))
			Position -= F * Speed; // Backward

		if (Input.IsDown(SDL_SCANCODE_A))
			Position -= R * Speed; // Left

		if (Input.IsDown(SDL_SCANCODE_D))
			Position += R * Speed; // Right
	}

	auto ProcessMouse() -> void
	{
		if (IgnoreNextMouseMotion)
		{
			IgnoreNextMouseMotion = false;
			return;
		}

		auto &Input = UServiceLocator::Get<UInputManger>();

		const auto Mouse = Input.GetMousePosition();

		constexpr float Sensitivity = 0.0025F;

		Yaw += Mouse.XRel * Sensitivity;
		Pitch -= Mouse.YRel * Sensitivity;

		Pitch = glm::clamp(Pitch, glm::radians(-89.0F), glm::radians(89.0F));
	}

	auto ResetMouseIgnore() -> void
	{
		IgnoreNextMouseMotion = true;
	}
};
