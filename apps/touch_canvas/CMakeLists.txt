add_executable(touch_canvas main.c)
target_link_libraries(touch_canvas PRIVATE raylib)
set_target_properties(touch_canvas PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

if(WIN32)
    set_target_properties(touch_canvas PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
