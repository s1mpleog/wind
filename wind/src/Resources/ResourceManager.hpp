#pragma once

// i want it to be like this
// TextureHandle texture = resources<Texture>.load("assets/texture.png")
// ShaderHandle shader = resources<Shader>.load(device, "assets/shader.vert")
// where, Texture and Shader is a handle not underlying resource
// conceptually struct Handle { TU32 index; TU32 generation }
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
	TU32 Index{};
};

struct Texture;
struct Shader;

using TExtureHandle = FHandle<Texture>;
using TShaderHandle = FHandle<Shader>;
using TVertexHandle = FHandle<TU32>;
using TIndexHandle = FHandle<TU32>;
using TModelHandle = FHandle<TU32>;
using TBufferHandle = FHandle<TU32>;
using TDynamicBufferHandle = FHandle<TU32>;

static_assert(sizeof(TExtureHandle) == 4);
static_assert(sizeof(TShaderHandle) == 4);

static_assert(!std::is_same_v<TExtureHandle, TShaderHandle>);

struct FMeshHandle
{
	TU32 Index{};
};

class UResourceManager
{
  public:
	UResourceManager(const UResourceManager &) = delete;
	auto operator=(const UResourceManager &) -> UResourceManager & = delete;

	UResourceManager(UResourceManager &&) = default;
	auto operator=(UResourceManager &&) -> UResourceManager & = default;

	WIND_NODISCARD static auto Create(const FVulkanContext *Context) WIND_NOEXCEPT -> TWindResult<UResourceManager>;

	WIND_NODISCARD auto LoadShader(const vk::raii::Device &Device, std::string_view ShaderPath) WIND_NOEXCEPT
	    -> TWindResult<TShaderHandle>;
	WIND_NODISCARD auto GetShader(TShaderHandle Handle) WIND_NOEXCEPT -> TWindResult<vk::raii::ShaderModule *>;
	WIND_NODISCARD auto GetShaderUnchecked(TShaderHandle Handle) WIND_NOEXCEPT -> vk::raii::ShaderModule *;
	auto DestroyShader(TShaderHandle Handle) WIND_NOEXCEPT -> void;

	WIND_NODISCARD auto CreateDefaultDepthImage(TU32 Width, TU32 Height) WIND_NOEXCEPT -> TWindResult<void>;
	WIND_NODISCARD auto GetDefaultDepthImageView() const WIND_NOEXCEPT -> const vk::raii::ImageView &;

	WIND_NODISCARD auto LoadModel(std::string_view TexturePath) WIND_NOEXCEPT -> TWindResult<TModelHandle>;
	// TODO: TWindResult does not works with reference fix that for now return pointer
	WIND_NODISCARD auto GetModel(TModelHandle Handle) WIND_NOEXCEPT -> TWindResult<const FModel *>;
	WIND_NODISCARD auto GetModelUnchecked(TModelHandle Handle) WIND_NOEXCEPT -> const FModel *;

	WIND_NODISCARD auto RecreateDefaultDepthImage(TU32 Width, TU32 Height) WIND_NOEXCEPT -> TWindResult<void>;

	WIND_NODISCARD auto CreateVertexBuffer(std::span<const std::byte> Vertices) WIND_NOEXCEPT
	    -> TWindResult<TBufferHandle>;
	WIND_NODISCARD auto CreateIndexBuffer(std::span<const std::byte> Indices) -> TWindResult<TBufferHandle>;
	WIND_NODISCARD auto GetBuffer(TBufferHandle Handle) const -> TWindResult<const FAllocatedBuffer *>;
	WIND_NODISCARD auto GetBufferUnchecked(TBufferHandle Handle) const -> const FAllocatedBuffer *;

	WIND_NODISCARD auto
	CreateDynamicBuffer(TU32 Size, vk::BufferUsageFlagBits Usage = vk::BufferUsageFlagBits::eUniformBuffer) WIND_NOEXCEPT
	    -> TWindResult<TDynamicBufferHandle>;

	WIND_NODISCARD auto CreateDynamicUniformBuffer(TU32 Size) WIND_NOEXCEPT -> TWindResult<TDynamicBufferHandle>;

	WIND_NODISCARD auto GetMappedData(TDynamicBufferHandle Handle) WIND_NOEXCEPT -> TWindResult<void *>;
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
