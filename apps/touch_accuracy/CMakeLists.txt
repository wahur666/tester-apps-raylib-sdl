add_executable(touch_accuracy main.c)
target_link_libraries(touch_accuracy PRIVATE raylib)
set_target_properties(touch_accuracy PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

if(WIN32)
    set_target_properties(touch_accuracy PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
