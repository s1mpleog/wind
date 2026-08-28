cmake_minimum_required(VERSION 3.28)

project(Wind
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

option(WIND_VALIDATION
    "Enable Vulkan validation layers"
    OFF
)

option(WIND_RENDERER_WARNINGS_AS_ERRORS
    "Treat compiler warnings as errors"
    OFF
)

option(WIND_LOG_ENABLE
    "Enable logging for creation and shutdown of subsystems"
    OFF
)


add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty
    ${CMAKE_BINARY_DIR}/ThirdParty
)


find_package(Vulkan REQUIRED)


add_executable(wind)

target_sources(wind
    PRIVATE
        src/Main.cpp
        src/Engine.cpp

        src/platform/Window.cpp

        src/vulkan/core/Context.cpp
        src/vulkan/core/Instance.cpp
        src/vulkan/core/ValidationLayer.cpp
        src/vulkan/core/Device.cpp
        src/vulkan/core/Swapchain.cpp

        src/vulkan/frame/FrameContext.cpp

        src/vulkan/graphics/Pipeline.cpp
        src/vulkan/Renderer.cpp

        src/vulkan/memory/Allocator.cpp

        src/resources/ResourceManager.cpp
        src/resources/DescriptorManager.cpp
        src/resources/Builtin.cpp

        src/input/InputManager.cpp

        src/scene/Scene.cpp

        src/VmaImpl.cpp
)

target_include_directories(wind
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(wind
    PRIVATE
        SDL3::SDL3
        spdlog::spdlog
        cpptrace::cpptrace
        glm::glm

        Vulkan::Headers
        Vulkan::Vulkan

        GPUOpen::VulkanMemoryAllocator
)

target_precompile_headers(wind
    PRIVATE
        <vulkan/vulkan.hpp>
        <vulkan/vulkan_raii.hpp>
        <spdlog/spdlog.h>
        <ranges>

        "${CMAKE_CURRENT_SOURCE_DIR}/src/Config.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/ExpectedUtil.hpp"
)

target_compile_options(wind
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -fno-exceptions
        -Wno-gnu-statement-expression-from-macro-expansion
)

target_compile_definitions(wind
    PRIVATE
        VULKAN_HPP_NO_EXCEPTIONS
        VULKAN_HPP_USE_STD_EXPECTED
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        FMT_EXCEPTIONS=0
        SPDLOG_NO_EXCEPTIONS
)

if(WIN32)
    target_compile_definitions(wind
        PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
    )
endif()

if(WIND_LOG_ENABLE)
    target_compile_definitions(wind
        PRIVATE
            WIND_LOG_ENABLE
    )
endif()

if(WIND_VALIDATION)
    target_compile_definitions(wind
        PRIVATE
            WIND_VULKAN_VALIDATION
    )
endif()

if(WIND_RENDERER_WARNINGS_AS_ERRORS)
    target_compile_options(wind
        PRIVATE
            -Werror
    )
endif()

find_program(CCACHE_PROGRAM ccache)

if(CCACHE_PROGRAM)
    message(STATUS "ccache          : ${CCACHE_PROGRAM}")

    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
else()
    message(STATUS "ccache          : not found")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(WIND_SHADER_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders"
)

set(WIND_SHADER_OUTPUT_DIR
    "${CMAKE_BINARY_DIR}/assets/shaders"
)

set(WIND_SHADER_SCRIPT
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/shaders.py"
)

file(GLOB_RECURSE WIND_SHADER_SOURCES
    CONFIGURE_DEPENDS
    "${WIND_SHADER_SOURCE_DIR}/*.vert"
    "${WIND_SHADER_SOURCE_DIR}/*.frag"
    "${WIND_SHADER_SOURCE_DIR}/*.comp"
    "${WIND_SHADER_SOURCE_DIR}/*.geom"
    "${WIND_SHADER_SOURCE_DIR}/*.tesc"
    "${WIND_SHADER_SOURCE_DIR}/*.tese"
    "${WIND_SHADER_SOURCE_DIR}/*.mesh"
    "${WIND_SHADER_SOURCE_DIR}/*.task"
    "${WIND_SHADER_SOURCE_DIR}/*.rgen"
    "${WIND_SHADER_SOURCE_DIR}/*.rchit"
    "${WIND_SHADER_SOURCE_DIR}/*.rmiss"
    "${WIND_SHADER_SOURCE_DIR}/*.rahit"
    "${WIND_SHADER_SOURCE_DIR}/*.rint"
    "${WIND_SHADER_SOURCE_DIR}/*.rcall"
)

set(WIND_COMPILED_SHADERS)

foreach(SHADER_SOURCE ${WIND_SHADER_SOURCES})

    file(RELATIVE_PATH SHADER_RELATIVE
        "${WIND_SHADER_SOURCE_DIR}"
        "${SHADER_SOURCE}"
    )

    set(SHADER_OUTPUT
        "${WIND_SHADER_OUTPUT_DIR}/${SHADER_RELATIVE}.spv"
    )

    add_custom_command(
        OUTPUT
            "${SHADER_OUTPUT}"

        COMMAND
            ${Python3_EXECUTABLE}
            "${WIND_SHADER_SCRIPT}"
            "${SHADER_SOURCE}"
            "${SHADER_OUTPUT}"

        DEPENDS
            "${SHADER_SOURCE}"
            "${WIND_SHADER_SCRIPT}"

        COMMENT
            "Compiling shader ${SHADER_RELATIVE}"

        VERBATIM
    )

    list(APPEND
        WIND_COMPILED_SHADERS
        "${SHADER_OUTPUT}"
    )

endforeach()

add_custom_target(WindShaders
    DEPENDS
        ${WIND_COMPILED_SHADERS}
)

add_dependencies(wind WindShaders)


set(WIND_MODEL_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/models"
)

set(WIND_MODEL_OUTPUT_DIR
    "${CMAKE_BINARY_DIR}/assets/models"
)

file(GLOB_RECURSE WIND_MODEL_SOURCES
    CONFIGURE_DEPENDS
    "${WIND_MODEL_SOURCE_DIR}/*.wind"
)

set(WIND_MODEL_OUTPUTS)

foreach(MODEL_SOURCE ${WIND_MODEL_SOURCES})

    file(RELATIVE_PATH MODEL_RELATIVE
        "${WIND_MODEL_SOURCE_DIR}"
        "${MODEL_SOURCE}"
    )

    set(MODEL_OUTPUT
        "${WIND_MODEL_OUTPUT_DIR}/${MODEL_RELATIVE}"
    )

    add_custom_command(
        OUTPUT
            "${MODEL_OUTPUT}"

        COMMAND
            ${CMAKE_COMMAND} -E make_directory
            "${WIND_MODEL_OUTPUT_DIR}"

        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${MODEL_SOURCE}"
            "${MODEL_OUTPUT}"

        DEPENDS
            "${MODEL_SOURCE}"

        COMMENT
            "Copying model ${MODEL_RELATIVE}"

        VERBATIM
    )

    list(APPEND WIND_MODEL_OUTPUTS
        "${MODEL_OUTPUT}"
    )

endforeach()

add_custom_target(WindModels
    ALL
    DEPENDS
        ${WIND_MODEL_OUTPUTS}
)

add_dependencies(wind WindModels)


string(TOLOWER "${CMAKE_BUILD_TYPE}" WIND_BUILD_TYPE_LOWER)

if(WIND_BUILD_TYPE_LOWER STREQUAL "debug")

    target_compile_definitions(wind
        PRIVATE
            RENDERER_DEBUG
    )

elseif(WIND_BUILD_TYPE_LOWER STREQUAL "release")

    target_compile_definitions(wind
        PRIVATE
            RENDERER_RELEASE
    )

endif()

add_custom_command(
    TARGET wind
    POST_BUILD

    COMMAND
        ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:wind>/assets"

    COMMENT "Copying assets to build directory..."
    VERBATIM
)

message(STATUS "")
message(STATUS "================ Wind Configuration ================")

message(STATUS "System          : ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Compiler        : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C++ Standard    : C++${CMAKE_CXX_STANDARD}")
message(STATUS "Generator       : ${CMAKE_GENERATOR}")
message(STATUS "Build Type      : ${CMAKE_BUILD_TYPE}")

message(STATUS "Vulkan          : ${Vulkan_VERSION}")
message(STATUS "Validation      : ${WIND_VALIDATION}")

message(STATUS "Logging         : ${WIND_LOG_ENABLE}")
message(STATUS "Warnings Errors : ${WIND_RENDERER_WARNINGS_AS_ERRORS}")

message(STATUS "Source Dir      : ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Build Dir       : ${CMAKE_BINARY_DIR}")
message(STATUS "Asset Dir       : ${CMAKE_CURRENT_SOURCE_DIR}/assets")
message(STATUS "Runtime Assets  : ${CMAKE_BINARY_DIR}/assets")

message(STATUS "Python          : ${Python3_EXECUTABLE}")
message(STATUS "Python Version  : ${Python3_VERSION}")
message(STATUS "GLSLC           : ${GLSLC_EXECUTABLE}")

message(STATUS "====================================================")
message(STATUS "")
