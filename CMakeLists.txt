cmake_minimum_required(VERSION 3.30)
project(SwiftRender)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

FILE(GLOB_RECURSE Sources Source/*.cpp)

add_library(SwiftRender STATIC)
target_include_directories(SwiftRender PUBLIC Include)
target_sources(SwiftRender PUBLIC ${Sources})
target_precompile_headers(SwiftRender PUBLIC Include/PCH.hpp)

add_subdirectory(External)

option(WithExample "Build the example project" ON)

if (WithExample)
    add_subdirectory(Example)
endif ()