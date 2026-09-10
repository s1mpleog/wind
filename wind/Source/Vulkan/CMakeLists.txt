add_library(WindVulkan STATIC)

wind_target_defaults(WindVulkan)

if(MSVC)
  target_compile_options(WindVulkan PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:
            /FI${PROJECT_SOURCE_DIR}/../ThirdParty/volk/volk.h
        >
    )
else()
  target_compile_options(WindVulkan PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:
            -include${PROJECT_SOURCE_DIR}/../ThirdParty/volk/volk.h
        >
    )
endif()

target_compile_definitions(WindVulkan
    PRIVATE
        VULKAN_HPP_NO_EXCEPTIONS
        VULKAN_HPP_USE_STD_EXPECTED
        VK_NO_PROTOTYPES
        VP_USE_OBJECT=1
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=0
)

target_sources(WindVulkan
    PRIVATE
        Private/VulkanCore.cpp
        ${CMAKE_SOURCE_DIR}/Generated/VulkanProfiles/vulkan_profiles.cpp
        Private/VulkanDevice.cpp
        Private/VulkanDebug.cpp
        Private/VulkanQueue.cpp
        Private/VulkanSwapchain.cpp
        Private/VulkanSynchronization.cpp
        Private/VulkanCommandBuffer.cpp
        Private/VulkanContext.cpp
)

target_include_directories(WindVulkan
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
        ${CMAKE_SOURCE_DIR}/Generated/VulkanProfiles

    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

if(WIN32)
  target_sources(WindVulkan PRIVATE Private/Windows/VulkanWindows.cpp)
elseif(UNIX)
  target_sources(WindVulkan PRIVATE Private/Linux/VulkanLinux.cpp)
endif()

target_link_libraries(WindVulkan
    PUBLIC
        Vulkan::Headers
        SDL3::SDL3

    PRIVATE
        WindCore
        #Vulkan::Vulkan
        volk
        spdlog::spdlog
)
