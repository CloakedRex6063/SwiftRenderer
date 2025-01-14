cmake_minimum_required(VERSION 3.29)
project(SwiftRender)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

FILE(GLOB_RECURSE Sources Source/*.cpp)

add_library(SwiftRender STATIC)
target_include_directories(SwiftRender PUBLIC Include PRIVATE Source)
target_sources(SwiftRender PUBLIC ${Sources})
target_precompile_headers(SwiftRender PUBLIC Include/SwiftPCH.hpp)

if (WIN32)
    target_compile_definitions(SwiftRender PUBLIC SWIFT_WINDOWS)
elseif (UNIX AND NOT APPLE)
    target_compile_definitions(SwiftRender PUBLIC SWIFT_LINUX)
endif ()

add_subdirectory(External)
add_subdirectory(Shaders)
add_dependencies(SwiftRender SwiftCompileShaders)

option(SwiftWithExample "Build the example project" OFF)
option(SwiftImGui "Support Imgui with swift" OFF)
option(SwiftImGuiGLFW "Support GLFW backend for imgui" OFF)

if (SwiftWithExample)
    add_subdirectory(Example)
endif ()

if (SwiftImGui)
    target_compile_definitions(SwiftRender PUBLIC SWIFT_IMGUI)
endif ()

if (SwiftImGuiGLFW)
    target_compile_definitions(SwiftRender PUBLIC SWIFT_IMGUI_GLFW)
endif ()

