cmake_minimum_required(VERSION 3.28)
project(wind VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

option(WIND_VALIDATION "Enable Vulkan validation layers" ON)
option(WIND_RENDERER_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(WIND_LOG_ENABLE "Enable logging for creation and shutdown of sub-systems and useful stuffs" OFF)

include(FetchContent)

FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG main
  GIT_SHALLOW TRUE
)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.17.0
  GIT_SHALLOW TRUE
)

FetchContent_Declare(
  cpptrace
  GIT_REPOSITORY https://github.com/jeremy-rifkin/cpptrace.git
  GIT_TAG main
  GIT_SHALLOW TRUE
)

set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.3
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    VulkanMemoryAllocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG       v3.4.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(SDL3 spdlog cpptrace glm VulkanMemoryAllocator)

find_package(Vulkan REQUIRED)

add_executable(wind
  src/main.cc
  src/platform/window.cc
  src/engine.cc
  src/vulkan/core/context.cc
  src/vulkan/core/instance.cc
  src/vulkan/core/validation_layer.cc
  src/vulkan/core/device.cc
  src/vulkan/core/swapchain.cc
  src/vulkan/frame/frame_context.cc
  src/resources/resource_manager.cc
  src/vulkan/graphics/pipeline.cc
  src/vulkan/renderer.cc
  src/vulkan/memory/allocator.cc
  src/vma_impl.cc
)

target_include_directories(wind PRIVATE ${CMAKE_BINARY_DIR}/generated)

target_include_directories(wind PRIVATE src)

target_link_libraries(wind PRIVATE
  SDL3::SDL3
  spdlog::spdlog
  cpptrace::cpptrace
  Vulkan::Vulkan
  glm::glm
  GPUOpen::VulkanMemoryAllocator
)

target_precompile_headers(wind PRIVATE
    <vulkan/vulkan.hpp>
    <vulkan/vulkan_raii.hpp>
    <spdlog/spdlog.h>
    <ranges>
    "${CMAKE_CURRENT_SOURCE_DIR}/src/config.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/expected_util.hpp"
)

target_compile_options(wind PRIVATE
  -Wall -Wextra -Wpedantic
  -fno-exceptions
  # -fsanitize=address
  # -fno-omit-frame-pointer
  -Wno-gnu-statement-expression-from-macro-expansion
)

# target_link_options(wind PRIVATE
#     -fsanitize=address
# )

target_compile_definitions(wind PRIVATE
  VULKAN_HPP_NO_EXCEPTIONS
  VULKAN_HPP_USE_STD_EXPECTED
  GLM_FORCE_DEPTH_ZERO_TO_ONE
  FMT_EXCEPTIONS=0
  SPDLOG_NO_EXCEPTIONS
)

if(WIN32)
  target_compile_definitions(wind PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" _build_type_lower)
if(_build_type_lower STREQUAL "debug")
  target_compile_definitions(wind PRIVATE RENDERER_DEBUG)
elseif(_build_type_lower STREQUAL "release")
  target_compile_definitions(wind PRIVATE RENDERER_RELEASE)
endif()

if(WIND_LOG_ENABLE)
  target_compile_definitions(wind PRIVATE WIND_LOG_ENABLE)
endif()

if(WIND_VALIDATION)
  target_compile_definitions(wind PRIVATE WIND_VULKAN_VALIDATION)
endif()

if(RENDERER_WARNINGS_AS_ERRORS)
  target_compile_options(wind PRIVATE -Werror)
endif()

add_custom_command(TARGET wind POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:wind>/assets"
    COMMENT "Copying assets to build directory..."
    VERBATIM
)

message(STATUS "=== Build Configuration ===")
message(STATUS "Compiler        : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C++ Standard    : c++26")
message(STATUS "Build type      : ${CMAKE_BUILD_TYPE}")
message(STATUS "Vulkan          : ${Vulkan_VERSION}")
message(STATUS "Validation      : ${VALIDATION}")
message(STATUS "Log enable      : ${LOG_ENABLE}")
