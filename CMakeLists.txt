cmake_minimum_required(VERSION 3.25)
project(touch_ray_demo C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples" FORCE)
set(PLATFORM SDL CACHE STRING "raylib platform backend" FORCE)

set(SDL_SHARED OFF CACHE BOOL "Build SDL shared library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build SDL static library" FORCE)
set(SDL_TESTS OFF CACHE BOOL "Build SDL tests" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "Build SDL examples" FORCE)

add_subdirectory(SDL-release-3.4.10 EXCLUDE_FROM_ALL)
add_subdirectory(raylib-6.0)

add_subdirectory(apps/touch_tracker)
add_subdirectory(apps/webcam_preview)
add_subdirectory(apps/touch_canvas)
add_subdirectory(apps/touch_accuracy)
