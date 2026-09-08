add_library(WindRenderer STATIC)

wind_target_defaults(WindRenderer)

target_compile_definitions(WindRenderer
    PRIVATE
        VULKAN_HPP_NO_EXCEPTIONS
        VULKAN_HPP_USE_STD_EXPECTED
        VK_NO_PROTOTYPES
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=0
)

target_sources(WindRenderer
    PRIVATE
      Private/test.cpp
)

target_include_directories(WindRenderer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public

    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

target_link_libraries(WindRenderer
    PUBLIC
        Vulkan::Headers

    PRIVATE
        WindCore
        WindVulkan
        #Vulkan::Vulkan
        volk
        spdlog::spdlog
)