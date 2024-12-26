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

    void HandleSubOptimalSwapchain(
        u32 graphicsFamily,
        const Context& context,
        Swapchain& swapchain,
        vk::Extent2D extent);

    void SubmitQueueHost(
        vk::Queue queue,
        vk::CommandBuffer commandBuffer,
        vk::Fence fence);

    void SubmitQueue(
        vk::Queue queue,
        vk::CommandBuffer commandBuffer,
        vk::Semaphore waitSemaphore,
        vk::PipelineStageFlags2 waitStageMask,
        vk::Semaphore signalSemaphore,
        vk::PipelineStageFlags2 signalStageMask,
        vk::Fence fence);

    inline void ResetFence(
        const vk::Device& device,
        const vk::Fence fence)
    {
        const auto result = device.resetFences(fence);
        VK_ASSERT(result, "Failed to reset fence");
    }
    inline void WaitFence(
        const vk::Device& device,
        const vk::Fence fence,
        const u64 timeout = std::numeric_limits<u64>::max())
    {
        const auto result = device.waitForFences(fence, true, timeout);
        VK_ASSERT(result, "Failed to wait fences");
    }

    inline void BeginOneTimeCommand(const vk::CommandBuffer commandBuffer)
    {
        auto result = commandBuffer.reset();
        VK_ASSERT(result, "Failed to reset command buffer");
        constexpr auto beginInfo =
            vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        result = commandBuffer.begin(beginInfo);
        VK_ASSERT(result, "Failed to begin command buffer");
    }
    inline void EndCommand(const vk::CommandBuffer commandBuffer)
    {
        const auto result = commandBuffer.end();
        VK_ASSERT(result, "Failed to end command buffer");
    }

    inline void UploadToBuffer(
        const Context& context,
        const void* data,
        const Buffer& buffer,
        const VkDeviceSize offset,
        const VkDeviceSize size)
    {
        const auto result =
            vmaCopyMemoryToAllocation(context.allocator, data, buffer.allocation, offset, size);
        assert(result == VK_SUCCESS && "Failed to copy memory to buffer");
    }

    Buffer UploadToImage(
        const Context& context,
        vk::CommandBuffer commandBuffer,
        u32 queueIndex,
        std::span<u8> imageData,
        vk::Extent3D extent,
        Image& image);

    void CopyBufferToImage(
        vk::CommandBuffer commandBuffer,
        vk::Buffer buffer,
        vk::Extent3D extent,
        vk::Image);

    void CopyImage(
        vk::CommandBuffer commandBuffer,
        vk::Image srcImage,
        vk::ImageLayout srcLayout,
        vk::Image dstImage,
        vk::ImageLayout dstLayout,
        vk::Extent2D extent);

    void BlitImage(
        vk::CommandBuffer commandBuffer,
        vk::Image srcImage,
        vk::ImageLayout srcLayout,
        vk::Image dstImage,
        vk::ImageLayout dstLayout,
        vk::Extent2D srcExtent,
        vk::Extent2D dstExtent);

    void UpdateDescriptorImage(
        vk::DescriptorSet set,
        vk::ImageView imageView,
        vk::Sampler sampler,
        u32 arrayElement,
        const vk::Device& device);

    void UpdateDescriptorSampler(
        vk::DescriptorSet set,
        vk::ImageView imageView,
        vk::Sampler sampler,
        u32 arrayElement,
        const vk::Device& device);

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
        VK_ASSERT(result, "Failed to set debug object name");
    }

    vk::ImageSubresourceRange GetImageSubresourceRange(
        vk::ImageAspectFlags aspectMask,
        u32 mipCount = 1);
    vk::ImageSubresourceLayers GetImageSubresourceLayers(vk::ImageAspectFlags aspectMask);

    vk::ImageMemoryBarrier2 ImageBarrier(
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        Image& image,
        vk::ImageAspectFlags flags);
    void PipelineBarrier(
        vk::CommandBuffer commandBuffer,
        vk::ArrayProxy<vk::ImageMemoryBarrier2> imageBarriers);

    inline vk::Extent2D To2D(const glm::uvec2 extent)
    {
        return vk::Extent2D(extent.x, extent.y);
    };
    inline vk::Extent3D To3D(const glm::uvec2 extent)
    {
        return vk::Extent3D(extent.x, extent.y, 1);
    }

    void ClearColorImage(
        const vk::CommandBuffer& commandBuffer,
        const Image& image);

    inline u32 PadAlignment(
        const u32 originalSize,
        const u32 alignment)
    {
        return (originalSize + alignment - 1) & ~(alignment - 1);
    }
}; // namespace Swift::Vulkan::Util
