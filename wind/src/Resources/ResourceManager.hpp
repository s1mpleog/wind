#pragma once

// i want it to be like this
// TextureHandle texture = resources<Texture>.load("assets/texture.png")
// ShaderHandle shader = resources<Shader>.load(device, "assets/shader.vert")
// where, Texture and Shader is a handle not underlying resource
// conceptually struct Handle { u32 index; u32 generation }
// provide options like get and delete do validation internally
// flow:
// TextureHandle tex = resource<Texture>.load("...");
// internally validate does file exists ? ...
// check the cache if hit -> increment index return handle
// load the file -> cache it -> increment handle data -> return handle

#include "Config.hpp"
#include "Resources/DescriptorManager.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Context.hpp"
#include "Vulkan/Memory/Allocator.hpp"
#include "Vulkan/Memory/ResourceTypes.hpp"

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

template <typename T_>
struct FHandle
{
	u32 Index{};
};

struct Texture;
struct Shader;

using TExtureHandle = FHandle<Texture>;
using TShaderHandle = FHandle<Shader>;
using TVertexHandle = FHandle<u32>;
using TIndexHandle = FHandle<u32>;
using TModelHandle = FHandle<u32>;
using TBufferHandle = FHandle<u32>;
using TDynamicBufferHandle = FHandle<u32>;

static_assert(sizeof(TExtureHandle) == 4);
static_assert(sizeof(TShaderHandle) == 4);

static_assert(!std::is_same_v<TExtureHandle, TShaderHandle>);

struct FMeshHandle
{
	u32 Index{};
};

class UResourceManager
{
  public:
	UResourceManager(const UResourceManager &) = delete;
	auto operator=(const UResourceManager &) -> UResourceManager & = delete;

	UResourceManager(UResourceManager &&) = default;
	auto operator=(UResourceManager &&) -> UResourceManager & = default;

	WIND_NODISCARD static auto Create(const FVulkanContext *Context) WIND_NOEXCEPT -> WindResult<UResourceManager>;

	WIND_NODISCARD auto LoadShader(const vk::raii::Device &Device, std::string_view ShaderPath) WIND_NOEXCEPT
	    -> WindResult<TShaderHandle>;
	WIND_NODISCARD auto GetShader(TShaderHandle Handle) WIND_NOEXCEPT -> WindResult<vk::raii::ShaderModule *>;
	WIND_NODISCARD auto GetShaderUnchecked(TShaderHandle Handle) WIND_NOEXCEPT -> vk::raii::ShaderModule *;
	auto DestroyShader(TShaderHandle Handle) WIND_NOEXCEPT -> void;

	WIND_NODISCARD auto CreateDefaultDepthImage(u32 Width, u32 Height) WIND_NOEXCEPT -> WindResult<void>;
	WIND_NODISCARD auto GetDefaultDepthImageView() const WIND_NOEXCEPT -> const vk::raii::ImageView &;

	WIND_NODISCARD auto LoadModel(std::string_view TexturePath) WIND_NOEXCEPT -> WindResult<TModelHandle>;
	// TODO: WindResult does not works with reference fix that for now return pointer
	WIND_NODISCARD auto GetModel(TModelHandle Handle) WIND_NOEXCEPT -> WindResult<const FModel *>;
	WIND_NODISCARD auto GetModelUnchecked(TModelHandle Handle) WIND_NOEXCEPT -> const FModel *;

	WIND_NODISCARD auto RecreateDefaultDepthImage(u32 Width, u32 Height) WIND_NOEXCEPT -> WindResult<void>;

	WIND_NODISCARD auto CreateVertexBuffer(std::span<const std::byte> Vertices) WIND_NOEXCEPT
	    -> WindResult<TBufferHandle>;
	WIND_NODISCARD auto CreateIndexBuffer(std::span<const std::byte> Indices) -> WindResult<TBufferHandle>;
	WIND_NODISCARD auto GetBuffer(TBufferHandle Handle) const -> WindResult<const FAllocatedBuffer *>;
	WIND_NODISCARD auto GetBufferUnchecked(TBufferHandle Handle) const -> const FAllocatedBuffer *;

	WIND_NODISCARD auto
	CreateDynamicBuffer(u32 Size, vk::BufferUsageFlagBits Usage = vk::BufferUsageFlagBits::eUniformBuffer) WIND_NOEXCEPT
	    -> WindResult<TDynamicBufferHandle>;

	WIND_NODISCARD auto CreateDynamicUniformBuffer(u32 Size) WIND_NOEXCEPT -> WindResult<TDynamicBufferHandle>;

	WIND_NODISCARD auto GetMappedData(TDynamicBufferHandle Handle) WIND_NOEXCEPT -> WindResult<void *>;
	WIND_NODISCARD auto GetMappedDataUnchecked(TDynamicBufferHandle Handle) WIND_NOEXCEPT -> void *;

	WIND_NODISCARD auto GetBindlessDescriptorSet() WIND_NOEXCEPT -> const vk::raii::DescriptorSet *
	{
		return MDescriptorManager.GetSet();
	}

	WIND_NODISCARD auto GetBindlessDescriptorLayout() WIND_NOEXCEPT -> const vk::raii::DescriptorSetLayout *
	{
		return MDescriptorManager.GetLayout();
	}

  private:
	UResourceManager(const FVulkanContext *Context, UGpuAllocator Allocator, UDescriptorManager DescriptorManager)
	    : MContext{Context}, MAllocator{std::move(Allocator)}, MDescriptorManager{std::move(DescriptorManager)} {};

	const FVulkanContext *MContext;
	std::vector<vk::raii::ShaderModule> MShaders;
	std::unordered_map<std::string, TShaderHandle> MShaderCache;
	UGpuAllocator MAllocator;
	std::unordered_map<std::string, TModelHandle> MModelCache;
	std::vector<FAllocatedTexture> MTexture;
	FAllocatedImage MDepthImage;
	UDescriptorManager MDescriptorManager;
	std::vector<FModel> MModels;
	std::vector<FAllocatedBuffer> MBuffers; // for things that don't uses models
};
