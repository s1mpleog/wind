function(wind_target_defaults TARGET)
  target_compile_features(${TARGET}
        PRIVATE
            #cxx_std_26
    )

  if(MSVC)
    target_compile_options(${TARGET}
            PRIVATE
                /W4
                /permissive-
                /EHs-c-
        )
  else()
    target_compile_options(${TARGET}
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -fno-exceptions
        )
  endif()

  target_compile_definitions(${TARGET}
        PRIVATE
            SPDLOG_NO_EXCEPTIONS
            FMT_EXCEPTIONS=0
    )
endfunction()
