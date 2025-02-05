add_subdirectory(VMA)
target_link_libraries(SwiftRender PUBLIC GPUOpen::VulkanMemoryAllocator)

add_subdirectory(DDS)
target_compile_definitions(SwiftRender PUBLIC DDS_USE_STD_FILESYSTEM DDS_CPP_17)
target_link_libraries(SwiftRender PRIVATE DDS)
target_include_directories(SwiftRender PUBLIC DDS/include)

find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
    message(STATUS "Vulkan SDK is installed")
    target_compile_definitions(SwiftRender PUBLIC SWIFT_VULKAN_SDK VULKAN_HPP_NO_EXCEPTIONS)
    target_link_libraries(SwiftRender PUBLIC Vulkan::Vulkan)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Enabling validation layers")
        target_compile_definitions(SwiftRender PUBLIC SWIFT_VULKAN_VALIDATION)
    endif ()
else()
    message(FATAL_ERROR "Vulkan SDK not found!")
endif()

FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm
        GIT_TAG 69b130c
)
FetchContent_MakeAvailable(glm)
target_link_libraries(SwiftRender PUBLIC glm::glm)

FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui
        GIT_TAG v1.91.7-docking
)
FetchContent_MakeAvailable(imgui)

if (SwiftImGui)
    FILE(GLOB ImGuiSources ${imgui_SOURCE_DIR}/*.cpp)
    add_library(imgui ${ImGuiSources})
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
    target_link_libraries(imgui PUBLIC Vulkan::Vulkan)
    target_sources(imgui PUBLIC ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp)
    target_link_libraries(SwiftRender PUBLIC imgui)
endif ()

if (SwiftImGuiGLFW)
    target_link_libraries(imgui PUBLIC glfw)
endif ()

