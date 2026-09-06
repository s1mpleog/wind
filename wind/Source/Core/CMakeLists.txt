add_library(WindCore STATIC)

wind_target_defaults(WindCore)

target_sources(WindCore
    PRIVATE
        Private/CrashHandler.cpp
)

target_include_directories(WindCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public

    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_CURRENT_SOURCE_DIR}/Internal
)

target_link_libraries(WindCore
    PUBLIC
        spdlog::spdlog
)

if(WIND_LOG_ENABLE)
  target_compile_definitions(WindCore
        PUBLIC
            WIND_LOG_ENABLE
    )
endif()
