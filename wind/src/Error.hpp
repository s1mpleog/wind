#pragma once

#include <SDL3/SDL_error.h>
#include <cpptrace/cpptrace.hpp>
#include <format>
#include <source_location>
#include <string>
#include <string_view>
#include <vulkan/vulkan.hpp>

#include "Types.hpp"
#include "Utils/Ansii.hpp"

enum class ErrorCode : u8
{
  // instance
  FailedToCreateInstance,
  FailedToCreateDebugMessenger,
  VulkanVersion14NotFound,
  DynamicRenderingNotSupported,
  Synchronization2FeatureNotSupported,

  // device
  NoSuitablePhysicalDevice,
  FailedToCreateDevice,
  ExtensionNotSupported,
  ValidationLayerNotSupported,
  LayerNotSupported,

  // surface & swapchain
  FailedToCreateSurface,
  FailedToCreateSwapchain,
  SwapchainOutOfDate,
  SwapchainSuboptimal,
  FailedToGetSurfaceCapabilities,
  FailedToGetSurfaceFormats,
  FailedToGetSurfacePresentModes,

  // image & memory
  FailedToCreateImageView,
  FailedToCreateImage,
  FailedToAllocateMemory,
  FailedToBindMemory,

  // commands
  FailedToCreateCommandPool,
  FailedToAllocateCommandBuffer,
  FailedToBeginCommandBuffer,
  FailedToEndCommandBuffer,

  // sync
  FailedToCreateFence,
  FailedToCreateSemaphore,
  FailedToWaitForFence,
  FailedToResetFence,

  // pipeline
  FailedToCreateRenderPass,
  FailedToCreateFramebuffer,
  FailedToCreatePipeline,
  FailedToCreatePipelineLayout,
  FailedToCreateShaderModule,
  FailedToCreateDescriptorSetLayout,
  FailedToCreateDescriptorPool,
  FailedToAllocateDescriptorSet,

  // buffers
  FailedToCreateBuffer,
  FailedToMapMemory,

  // sdl
  FailedToCreateWindow,
  FailedToInitSDL,

  InvalidWindowConfig,

  // queue
  FailedToSubmitQueue,
  FailedToPresentQueue,
  NoGraphicsQueueFamily,
  NoPresentQueueFamily,

  // shader / io
  FailedToLoadShader,
  ShaderCompilationFailed,
  ResourceNotFound,

  InternalError,
};

enum ErrorKind : u8
{
  Vulkan,
  SDL,
  Internal
};

struct WindError
{
  ErrorCode            code;
  ErrorKind            kind;
  vk::Result           vk_result;
  std::string_view     sdl_message;
  std::source_location location;
  cpptrace::stacktrace trace;

public:
  WIND_NODISCARD static auto vulkan(ErrorCode            code   = ErrorCode::InternalError,
                                    vk::Result           result = vk::Result::eErrorUnknown,
                                    std::source_location loc    = std::source_location::current()) -> WindError
  {
    return WindError{code, ErrorKind::Vulkan, result, {}, loc};
  }

  WIND_NODISCARD static auto sdl(ErrorCode code, std::source_location loc = std::source_location::current()) -> WindError
  {
    return WindError{code, ErrorKind::SDL, vk::Result::eSuccess, SDL_GetError(), loc};
  };

  WIND_NODISCARD static auto internal(ErrorCode            code = ErrorCode::InternalError,
                                      std::source_location loc  = std::source_location::current()) -> WindError
  {
    return {code, ErrorKind::Internal, vk::Result::eSuccess, {}, loc};
  }
  WIND_NODISCARD auto to_string() const -> std::string
  {
    std::string out;

#ifndef NDEBUG
    {
      switch(kind)
      {
        case ErrorKind::Vulkan:
          out += std::format("{}[VULKAN ERROR]{} ", BOLD_RED, RESET);
          break;
        case ErrorKind::SDL:
          out += std::format("{}[SDL ERROR]{} ", BOLD_RED, RESET);
          break;
        case ErrorKind::Internal:
          out += std::format("{}[INTERNAL ERROR]{} ", BOLD_RED, RESET);
          break;
      }
      // ── main message ───────────────────────────────────────────
      out += std::format("{}{}{}\n", WHITE, code_message(), RESET);
      // ── subsystem detail ───────────────────────────────────────
      switch(kind)
      {
        case ErrorKind::Vulkan:
          out += std::format("  {}VkResult:{} {}{}{}\n", DIM, RESET, YELLOW, vk::to_string(vk_result), RESET);
          break;
        case ErrorKind::SDL:
          if(!sdl_message.empty())
            out += std::format("  {}SDL:{} {}{}{}\n", DIM, RESET, YELLOW, sdl_message, RESET);
          break;
        case ErrorKind::Internal:
          break;
      }
      // ── source location ────────────────────────────────────────
      out += std::format("\n  {}Location{}  {}{}:{}{} in {}{}{}\n", CYAN, RESET, GRAY,
                         location.file_name(), location.line(), RESET, DIM, location.function_name(), RESET);
      // ── stack trace ────────────────────────────────────────────
      out += std::format("\n  {}Stack Trace{}\n", CYAN, RESET);
      for(size_t i = 0; i < trace.frames.size(); ++i)
      {
        const auto& frame = trace.frames[i];

        out += std::format("  {}#{:<3}{}", CYAN, i, RESET);
        out += std::format(" {}{:<60}{}", WHITE, frame.symbol, RESET);
        if(!frame.filename.empty())
          out += std::format(" {}{}:{}{}\n", GRAY, frame.filename, frame.line.value_or(0), RESET);
        else
          out += '\n';
      }
    }
#else
    {
      // ── plain output for release ───────────────────────────────
      out += std::format("[{}] {}",
                         kind == ErrorKind::Vulkan ? "VULKAN" :
                         kind == ErrorKind::SDL    ? "SDL" :
                                                     "INTERNAL",
                         code_message());
      switch(kind)
      {
        case ErrorKind::Vulkan:
          out += std::format(" [VkResult: {}]", vk::to_string(vk_result));
          break;
        case ErrorKind::SDL:
          if(!sdl_message.empty())
            out += std::format(" [SDL: {}]", sdl_message);
          break;
        case ErrorKind::Internal:
          break;
      }
    }
#endif

    return out;
  }

private:
  WindError(ErrorCode code, ErrorKind kind, vk::Result vk_result, std::string_view sdl_message, std::source_location location)
      : code{code}
      , kind{kind}
      , vk_result{vk_result}
      , sdl_message{sdl_message}
      , location{location}
      , trace{}
  {

#ifndef NDEBUG
    trace = cpptrace::generate_trace();
#endif
  };

  WIND_NODISCARD constexpr auto code_message() const WIND_NOEXCEPT -> std::string_view
  {
    switch(code)
    {
      // instance
      case ErrorCode::FailedToCreateInstance:
        return "Failed to create Vulkan instance — check driver installation and "
               "Vulkan SDK version";
      case ErrorCode::FailedToCreateDebugMessenger:
        return "Failed to create Vulkan debug messenger — validation layers may "
               "not be installed";

      case ErrorCode::VulkanVersion14NotFound:
        return "Vulkan loader 1.4 is required to run this app - try updating your Vulkan "
               "loader";

      case ErrorCode::LayerNotSupported:
        return "Failed to find requested layer - not supported";

      case ErrorCode::DynamicRenderingNotSupported:
        return "Dynamic Rendering is required to run this features - make sure "
               "your "
               "Vulkan version is 1.3 or above";
      case ErrorCode::Synchronization2FeatureNotSupported:
        return "This feature is required to run Application - make sure "
               "your "
               "Vulkan version is 1.3 or above";

      // device
      case ErrorCode::NoSuitablePhysicalDevice:
        return "No Vulkan-capable GPU found — update your drivers or verify "
               "Vulkan support for your card";
      case ErrorCode::FailedToCreateDevice:
        return "Failed to create Vulkan logical device — requested features or "
               "queues may not be supported";
      case ErrorCode::ExtensionNotSupported:
        return "Requested Vulkan extension is not supported by this device or "
               "instance";
      case ErrorCode::ValidationLayerNotSupported:
        return "Validation layer requested but not available — install the "
               "Vulkan SDK or disable validation";

      // surface & swapchain
      case ErrorCode::FailedToCreateSurface:
        return "Failed to create Vulkan surface — SDL3 window handle may be "
               "invalid";
      case ErrorCode::FailedToCreateSwapchain:
        return "Failed to create swapchain — surface capabilities or format may "
               "be unsupported";
      case ErrorCode::SwapchainOutOfDate:
        return "Swapchain is out of date — window was resized and swapchain must "
               "be recreated";
      case ErrorCode::SwapchainSuboptimal:
        return "Swapchain is suboptimal — consider recreating it for best "
               "performance";

      case ErrorCode::FailedToGetSurfaceCapabilities:
        return "Failed to query surface capabilities — surface may no longer be "
               "valid";
      case ErrorCode::FailedToGetSurfaceFormats:
        return "Failed to query surface formats — no compatible format found for "
               "this surface";
      case ErrorCode::FailedToGetSurfacePresentModes:
        return "Failed to query surface present modes — surface may not support "
               "presentation";

      // image & memory
      case ErrorCode::FailedToCreateImageView:
        return "Failed to create image view — image format or aspect flags may "
               "be incorrect";
      case ErrorCode::FailedToCreateImage:
        return "Failed to create Vulkan image — requested format or usage flags "
               "may be unsupported";
      case ErrorCode::FailedToAllocateMemory:
        return "Failed to allocate device memory — GPU may be out of memory";
      case ErrorCode::FailedToBindMemory:
        return "Failed to bind device memory to resource";

      // commands
      case ErrorCode::FailedToCreateCommandPool:
        return "Failed to create command pool — queue family index may be "
               "invalid";
      case ErrorCode::FailedToAllocateCommandBuffer:
        return "Failed to allocate command buffer from pool";
      case ErrorCode::FailedToBeginCommandBuffer:
        return "Failed to begin command buffer recording";
      case ErrorCode::FailedToEndCommandBuffer:
        return "Failed to end command buffer recording — commands may be "
               "unbalanced";

      // sync
      case ErrorCode::FailedToCreateFence:
        return "Failed to create synchronization fence";
      case ErrorCode::FailedToCreateSemaphore:
        return "Failed to create synchronization semaphore";
      case ErrorCode::FailedToWaitForFence:
        return "Failed waiting on fence — device may be lost";
      case ErrorCode::FailedToResetFence:
        return "Failed to reset fence";

      // pipeline
      case ErrorCode::FailedToCreateRenderPass:
        return "Failed to create render pass — attachment or subpass description "
               "may be invalid";
      case ErrorCode::FailedToCreateFramebuffer:
        return "Failed to create framebuffer — image view dimensions may not "
               "match render pass";
      case ErrorCode::FailedToCreatePipeline:
        return "Failed to create graphics pipeline — check shader modules and "
               "pipeline state";
      case ErrorCode::FailedToCreatePipelineLayout:
        return "Failed to create pipeline layout — descriptor set layouts may be "
               "invalid";
      case ErrorCode::FailedToCreateShaderModule:
        return "Failed to create shader module — SPIR-V bytecode may be "
               "malformed";
      case ErrorCode::FailedToCreateDescriptorSetLayout:
        return "Failed to create descriptor set layout";
      case ErrorCode::FailedToCreateDescriptorPool:
        return "Failed to create descriptor pool — pool sizes may be "
               "insufficient";
      case ErrorCode::FailedToAllocateDescriptorSet:
        return "Failed to allocate descriptor set — pool may be exhausted";

      // buffers
      case ErrorCode::FailedToCreateBuffer:
        return "Failed to create Vulkan buffer — usage flags or size may be "
               "invalid";
      case ErrorCode::FailedToMapMemory:
        return "Failed to map device memory — memory may not be host-visible";

      // sdl
      case ErrorCode::FailedToCreateWindow:
        return "Failed to create SDL3 window — check display server and SDL3 "
               "installation";
      case ErrorCode::FailedToInitSDL:
        return "Failed to initialize SDL3 — check SDL3 installation and system "
               "dependencies";

      // queue
      case ErrorCode::FailedToSubmitQueue:
        return "Failed to submit command buffer to queue — device may be lost";
      case ErrorCode::FailedToPresentQueue:
        return "Failed to present to queue — swapchain may need recreation";
      case ErrorCode::NoGraphicsQueueFamily:
        return "No graphics queue family found on selected physical device";
      case ErrorCode::NoPresentQueueFamily:
        return "No present queue family found — surface may not be supported by "
               "this device";

      // shader / io
      case ErrorCode::FailedToLoadShader:
        return "Failed to load shader file — check path and file permissions";
      case ErrorCode::ShaderCompilationFailed:
        return "Shader compilation failed — check GLSL source for errors";

      case ErrorCode::InternalError:
        return "Internal engine error — cannot continue";

      case ErrorCode::ResourceNotFound:
        return "Resource not found 404 - The requested file is not present";

      case ErrorCode::InvalidWindowConfig:
        return "Window configuration error - trying to create window with invalid config";

      default:
        return "Unknown error";
    }
  }
};
