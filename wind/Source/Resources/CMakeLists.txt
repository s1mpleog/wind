add_library(WindResources STATIC)

wind_target_defaults(WindResources)

# Temporary fix
if(MSVC)
  target_compile_options(WindResources PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:
            /FI${PROJECT_SOURCE_DIR}/../ThirdParty/volk/volk.h
        >
    )
else()
  target_compile_options(WindResources PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:
            -include${PROJECT_SOURCE_DIR}/../ThirdParty/volk/volk.h
        >
    )
endif()

target_compile_definitions(WindResources
    PRIVATE
        VULKAN_HPP_NO_EXCEPTIONS
        VULKAN_HPP_USE_STD_EXPECTED
        VK_NO_PROTOTYPES
)

target_sources(WindResources
    PRIVATE  
      Private/WindAssetLoader.hpp
)

target_include_directories(WindResources
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public

    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

target_link_libraries(WindResources
    PUBLIC
        Vulkan::Headers

    PRIVATE
        WindCore
        spdlog::spdlog
        glm::glm
)
