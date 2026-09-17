add_library(WindEngine STATIC)

wind_target_defaults(WindEngine)

# Temporary fix
if(MSVC)
  target_compile_options(WindEngine PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:
            /FI${PROJECT_SOURCE_DIR}/../ThirdParty/volk/volk.h
        >
    )
else()
  target_compile_options(WindEngine PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:
            -include${PROJECT_SOURCE_DIR}/../ThirdParty/volk/volk.h
        >
    )
endif()

target_compile_definitions(WindEngine
    PRIVATE
        VULKAN_HPP_NO_EXCEPTIONS
        VULKAN_HPP_USE_STD_EXPECTED
        VK_NO_PROTOTYPES
        VP_USE_OBJECT=1
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=0
)

target_sources(WindEngine
    PRIVATE  
      Private/Engine.cpp
)

target_include_directories(WindEngine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public

    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

target_link_libraries(WindEngine
    PUBLIC
        Vulkan::Headers

    PRIVATE
        WindCore
        WindApplicationCore
        WindVulkan
        WindRenderer
        #WindResources
        spdlog::spdlog
)
