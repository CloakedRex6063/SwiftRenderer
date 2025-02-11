cmake_minimum_required(VERSION 3.20)
project(SwiftRender)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

include(FetchContent)

FILE(GLOB_RECURSE Sources Source/*.cpp)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-fexperimental-library)
endif()

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

add_subdirectory(External)
if (SwiftWithExample)
    add_subdirectory(Examples)
endif ()

