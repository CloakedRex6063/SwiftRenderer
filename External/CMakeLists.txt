add_subdirectory(VulkanHeaders)
target_link_libraries(SwiftRender PUBLIC Vulkan::Headers)

add_subdirectory(GLM)
target_link_libraries(SwiftRender PUBLIC glm::glm)