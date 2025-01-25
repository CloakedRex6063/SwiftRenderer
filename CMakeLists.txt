cmake_minimum_required(VERSION 3.20)
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

option(SwiftWithExample "Build the example project" ON)
option(SwiftImGui "Support Imgui with swift" ON)
option(SwiftImGuiGLFW "Support GLFW backend for imgui" ON)

if (SwiftWithExample)
    add_subdirectory(Examples)
endif ()

if (SwiftImGui)
    target_compile_definitions(SwiftRender PUBLIC SWIFT_IMGUI)
endif ()

if (SwiftImGuiGLFW)
    target_compile_definitions(SwiftRender PUBLIC SWIFT_IMGUI_GLFW)
endif ()

add_subdirectory(External)
add_subdirectory(Shaders)
add_dependencies(SwiftRender SwiftCompileShaders)

