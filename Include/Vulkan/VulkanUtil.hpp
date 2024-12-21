#pragma once
#include "VulkanStructs.hpp"

namespace Swift::Vulkan
{
    struct Context;
}
namespace Swift::Vulkan::Util
{
    std::vector<u32> GetQueueFamilyIndices(
        vk::PhysicalDevice physicalDevice,
        vk::SurfaceKHR surface);
    void WaitFence(
        vk::Device device,
        vk::Fence fence,
        u64 timeout = std::numeric_limits<u64>::max());
    void HandleSubOptimalSwapchain(
        u32 graphicsFamily,
        const Context& context,
        Swapchain& swapchain,
        vk::Extent2D extent);
    void SubmitQueue(
        vk::Queue queue,
        vk::CommandBuffer commandBuffer,
        vk::Semaphore waitSemaphore,
        vk::PipelineStageFlags2 waitStageMask,
        vk::Semaphore signalSemaphore,
        vk::PipelineStageFlags2 signalStageMask,
        vk::Fence fence);

    template <typename T>
    static void NameObject(
        T object,
        const std::string_view label,
        const Context& context)
    {
#if defined(NDEBUG)
        return;
#endif
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};

        nameInfo.pObjectName = label.data();
        nameInfo.objectType = object.objectType;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(static_cast<typename T::CType>(object));
        
        auto result = context.device.setDebugUtilsObjectNameEXT(&nameInfo, context.dynamicLoader);
    }

    vk::ImageSubresourceRange GetImageSubresourceRange(
        vk::ImageAspectFlags aspectMask,
        u32 mipCount = 1);

    inline vk::Extent2D ToExtent2D(const glm::uvec2 extent)
    {
        return vk::Extent2D(extent.x, extent.y);
    };

    inline vk::Extent3D ToExtent3D(const glm::uvec2 extent)
    {
        return vk::Extent3D(extent.x, extent.y, 1);
    }
};  // namespace Swift::Vulkan::Util
