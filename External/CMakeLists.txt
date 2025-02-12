FetchContent_Declare(
        DDS
        GIT_REPOSITORY https://github.com/CloakedRex6063/DDS-Utility
        GIT_TAG ade8320
)
target_compile_definitions(SwiftRender PUBLIC DDS_USE_STD_FILESYSTEM DDS_CPP_17)
FetchContent_MakeAvailable(DDS)
target_link_libraries(SwiftRender PUBLIC DDS)

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
        vma
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
        GIT_TAG c788c52
)
FetchContent_MakeAvailable(vma)
target_compile_options(VulkanMemoryAllocator INTERFACE
        $<$<CXX_COMPILER_ID:MSVC>:/w>
        $<$<CXX_COMPILER_ID:GNU>:-w>
        $<$<CXX_COMPILER_ID:Clang>:-w>
)
target_link_libraries(SwiftRender PUBLIC GPUOpen::VulkanMemoryAllocator)
target_link_libraries(SwiftRender PRIVATE glfw)

