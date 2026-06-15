add_executable(touch_ray_demo main.c)
target_link_libraries(touch_ray_demo PRIVATE raylib)
set_target_properties(touch_ray_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

if(WIN32)
    set_target_properties(touch_ray_demo PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
