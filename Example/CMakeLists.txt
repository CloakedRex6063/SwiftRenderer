add_executable(Example)

FILE(GLOB_RECURSE Sources Source/*.cpp)
target_sources(Example PRIVATE ${Sources})

target_link_libraries(Example PRIVATE SwiftRender)

include(FetchContent)
FetchContent_Declare (
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 7b6aead9fb88b3623e3b3725ebb42670cbe4c579
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_COCOA OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)
target_link_libraries(Example PRIVATE glfw)

FetchContent_Declare (
        fastgltf
        GIT_REPOSITORY https://github.com/spnda/fastgltf
        GIT_TAG d0c89e5610129dc8192f66337d7fb180ff28fbe4
)
set(FASTGLTF_COMPILE_AS_CPP20 ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(fastgltf)

target_link_libraries(Example PRIVATE fastgltf::fastgltf)
