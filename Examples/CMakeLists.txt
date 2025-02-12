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

FetchContent_Declare (
        fastgltf
        GIT_REPOSITORY https://github.com/spnda/fastgltf
        GIT_TAG d0c89e5610129dc8192f66337d7fb180ff28fbe4
)
set(FASTGLTF_COMPILE_AS_CPP20 ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(fastgltf)

FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui
        GIT_TAG v1.91.7-docking
)
FetchContent_MakeAvailable(imgui)

find_package(Vulkan REQUIRED)
FILE(GLOB ImGuiSources ${imgui_SOURCE_DIR}/*.cpp)
add_library(imgui ${ImGuiSources})
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PRIVATE Vulkan::Vulkan)
target_sources(imgui PRIVATE ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp)
target_link_libraries(imgui PRIVATE glfw)

set(SOURCE_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/Resources")
set(DESTINATION_FOLDER "${CMAKE_CURRENT_BINARY_DIR}/Resources")

add_subdirectory(Shaders)
add_subdirectory(Showcase)
add_subdirectory(Utility)

add_custom_target(CopyResources)
add_custom_command(
        TARGET CopyResources POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${SOURCE_FOLDER} ${DESTINATION_FOLDER}
        COMMENT "Copying Resources folder"
)

set(CompressScript ${CMAKE_CURRENT_SOURCE_DIR}/Scripts/Python/Compress.py)
add_custom_target(CompressTextures)
add_custom_command(
        TARGET CompressTextures POST_BUILD
        COMMAND python ${CompressScript} "${CMAKE_CURRENT_BINARY_DIR}/Resources"
        COMMENT "Running Texture Compression"
)

set(UseDDSScript ${CMAKE_CURRENT_SOURCE_DIR}/Scripts/Python/UseDDS.py)
add_custom_target(UseDDS)
add_custom_command(
        TARGET UseDDS POST_BUILD
        COMMAND python ${UseDDSScript} "${CMAKE_CURRENT_BINARY_DIR}/Resources"
        COMMENT "Updating GLTF's to use DDS"
)

add_dependencies(Utility CompressTextures)
add_dependencies(Utility CompileShaders)
add_dependencies(Utility UseDDS)
add_dependencies(CompressTextures CopyResources)
add_dependencies(UseDDS CompressTextures)
