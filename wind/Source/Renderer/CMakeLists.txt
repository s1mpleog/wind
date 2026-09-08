add_library(WindRenderer STATIC)

wind_target_defaults(WindRenderer)

# Temporary fix
if(MSVC)
  target_compile_options(WindRenderer PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:
            /FI${PROJECT_SOURCE_DIR}/../ThirdParty/volk/volk.h
        >
    )
else()
  target_compile_options(WindRenderer PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:
            -include${PROJECT_SOURCE_DIR}/../ThirdParty/volk/volk.h
        >
    )
endif()

target_compile_definitions(WindRenderer
    PRIVATE
        VULKAN_HPP_NO_EXCEPTIONS
        VULKAN_HPP_USE_STD_EXPECTED
        VK_NO_PROTOTYPES
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=0
)

target_sources(WindRenderer
    PRIVATE  
      Private/Renderer.cpp
      Private/FrameContext.cpp
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
        spdlog::spdlog
)
