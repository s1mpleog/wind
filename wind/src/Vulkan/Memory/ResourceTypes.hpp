#pragma once

#include "Vulkan/Graphics/PipelineConfig.hpp"

#include <glm/glm.hpp>
#include <optional>
#include <utility>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

struct FAllocatedBuffer
{
	VkBuffer Buffer{};
	VmaAllocation Allocation{};
	VmaAllocator Allocator{};
	void *Mapped{nullptr};

	FAllocatedBuffer() = default;
	FAllocatedBuffer(const FAllocatedBuffer &) = delete;
	auto operator=(const FAllocatedBuffer &) -> FAllocatedBuffer & = delete;

	FAllocatedBuffer(VkBuffer Buffer, VmaAllocation Allocation, VmaAllocator Allocator, void *Mapped = nullptr)
	    : Buffer(Buffer), Allocation(Allocation), Allocator(Allocator), Mapped{Mapped}
	{
	}

	FAllocatedBuffer(FAllocatedBuffer &&Other) noexcept
	    : Buffer(std::exchange(Other.Buffer, VK_NULL_HANDLE)), Allocation(std::exchange(Other.Allocation, nullptr)),
	      Allocator(Other.Allocator), Mapped(std::exchange(Other.Mapped, nullptr)) {};

	auto Reset() WIND_NOEXCEPT -> void
	{
		if (Buffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(Allocator, Buffer, Allocation);

			Buffer = VK_NULL_HANDLE;
			Allocation = nullptr;
			Allocation = nullptr;
			Mapped = nullptr;
		}
	}

	auto operator=(FAllocatedBuffer &&Other) noexcept -> FAllocatedBuffer &
	{
		if (this != &Other)
		{
			Reset();

			Buffer = std::exchange(Other.Buffer, VK_NULL_HANDLE);
			Allocation = std::exchange(Other.Allocation, nullptr);
			Allocator = std::exchange(Other.Allocator, nullptr);
			Mapped = std::exchange(Other.Mapped, nullptr);
		}

		return *this;
	}

	~FAllocatedBuffer()
	{
		if (Buffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(Allocator, Buffer, Allocation);
			spdlog::info("VkBuffer destroyed successfully");
		}
	}
};

struct FSubMesh
{
	TU32 IndexCount{};
	TU32 IndexOffset{};
	TU32 MaterialIndex{};
};

struct FMesh
{
	FAllocatedBuffer VertexBuffer;
	FAllocatedBuffer IndexBuffer;
	FAllocatedBuffer Normals;
	FAllocatedBuffer Uvs;
	FAllocatedBuffer Tangents;

	TU32 IndexCount{};
	TU32 VertexCount{};
	TU32 NormalCount{};
	TU32 UvCount{};
	TU32 TangentCount{};

	std::vector<FSubMesh> SubMeshes;
};

struct FGpuMaterial
{
	std::optional<TU32> AlbedoTexture;
	std::optional<TU32> NormalTexture;
	std::optional<TU32> MetallicRoughnessTexture;

	float Metallic{};
	float Roughness{};
	glm::vec4 BaseColor{};
};

struct FModel
{
	FMesh Mesh{};
	std::vector<FGpuMaterial> Materials;
};

struct FAllocatedImage
{
	VkImage Image{VK_NULL_HANDLE};
	vk::raii::ImageView ImageView{nullptr};
	VmaAllocator Allocator{VK_NULL_HANDLE};
	VmaAllocation Allocation{VK_NULL_HANDLE};
	vk::Format Format{};
	vk::Extent2D Dimension{};

	FAllocatedImage(VkImage Image, vk::raii::ImageView ImageView, VmaAllocator Allocator, VmaAllocation Allocation,
	                vk::Format Format, vk::Extent2D Dimension)
	    : Image{Image}, ImageView{std::move(ImageView)}, Allocator{Allocator}, Allocation{Allocation}, Format{Format},
	      Dimension{Dimension}
	{
	}

	auto DestroyImage() WIND_NOEXCEPT -> void
	{
		ImageView = nullptr;

		if (Image != VK_NULL_HANDLE)
		{
			vmaDestroyImage(Allocator, Image, Allocation);
		}

		Image = VK_NULL_HANDLE;
		Allocation = VK_NULL_HANDLE;
	}

	FAllocatedImage() = default;
	FAllocatedImage(const FAllocatedImage &) = delete;
	auto operator=(const FAllocatedImage &) -> FAllocatedImage & = delete;

	FAllocatedImage(FAllocatedImage &&Other) WIND_NOEXCEPT
	    : Image{std::exchange(Other.Image, VK_NULL_HANDLE)},
	      ImageView{std::move(Other.ImageView)},
	      Allocator{std::exchange(Other.Allocator, VK_NULL_HANDLE)},
	      Allocation{std::exchange(Other.Allocation, VK_NULL_HANDLE)},
	      Format{Other.Format},
	      Dimension{Other.Dimension}
	{
	}

	auto operator=(FAllocatedImage &&Other) WIND_NOEXCEPT->FAllocatedImage &
	{
		if (this == &Other)
			return *this;

		// Destroy our current VMA-owned image first.
		if (Allocation != VK_NULL_HANDLE && Allocator != VK_NULL_HANDLE)
		{
			vmaDestroyImage(Allocator, Image, Allocation);
		}

		Image = std::exchange(Other.Image, VK_NULL_HANDLE);
		Allocation = std::exchange(Other.Allocation, VK_NULL_HANDLE);
		Allocator = std::exchange(Other.Allocator, VK_NULL_HANDLE);

		ImageView = std::move(Other.ImageView);

		Format = Other.Format;
		Dimension = Other.Dimension;

		return *this;
	}

	~FAllocatedImage()
	{
		if (Allocation != VK_NULL_HANDLE && Allocator != VK_NULL_HANDLE)
		{
			vmaDestroyImage(Allocator, Image, Allocation);
			spdlog::info("texture destroyed successfully");
		}
	}
};

struct FAllocatedTexture
{
	FAllocatedImage Image;
	vk::raii::Sampler Sampler{nullptr};

	FAllocatedTexture(FAllocatedImage Image, vk::raii::Sampler Sampler)
	    : Image{std::move(Image)}, Sampler{std::move(Sampler)}
	{
	}

	FAllocatedTexture(const FAllocatedTexture &) = delete;
	auto operator=(const FAllocatedTexture &) -> FAllocatedTexture & = delete;

	FAllocatedTexture(FAllocatedTexture &&) WIND_NOEXCEPT = default;
	auto operator=(FAllocatedTexture &&) WIND_NOEXCEPT->FAllocatedTexture & = default;

	~FAllocatedTexture() = default;
};

struct FTextureData
{
	std::span<const std::byte> Pixels;
	vk::Extent3D Dimensions{};
	EFormat Format{};
};

struct FBufferData
{
	std::span<const std::byte> Data;
	vk::BufferUsageFlags Usage{vk::BufferUsageFlagBits::eVertexBuffer};
};
