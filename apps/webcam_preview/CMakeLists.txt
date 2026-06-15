add_executable(webcam_preview main.c)
target_link_libraries(webcam_preview PRIVATE raylib SDL3::SDL3)
set_target_properties(webcam_preview PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

if(WIN32)
    set_target_properties(webcam_preview PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
