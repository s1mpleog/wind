add_library(WindApplicationCore STATIC)

wind_target_defaults(WindApplicationCore)

target_sources(WindApplicationCore
    PRIVATE
        Private/GenericPlatform/GenericWindow.cpp
        Private/LinuxWindow.cpp
)

target_include_directories(WindApplicationCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public

    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

target_link_libraries(WindApplicationCore
    PRIVATE
        WindCore
        SDL3::SDL3
        spdlog::spdlog
)
