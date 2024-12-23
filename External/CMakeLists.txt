add_subdirectory(GLM)
target_link_libraries(SwiftRender PUBLIC glm::glm)

add_subdirectory(VMA)
target_link_libraries(SwiftRender PUBLIC GPUOpen::VulkanMemoryAllocator)

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