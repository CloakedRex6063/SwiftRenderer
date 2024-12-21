add_executable(Example)
target_sources(Example PRIVATE Source/main.cpp)

include(FetchContent)
FetchContent_Declare (
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 7b6aead9fb88b3623e3b3725ebb42670cbe4c579
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

target_link_libraries(Example PRIVATE SwiftRender)
target_link_libraries(Example PRIVATE glfw)