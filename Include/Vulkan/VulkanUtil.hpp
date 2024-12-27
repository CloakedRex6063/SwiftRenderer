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

    vk::Extent3D GetMipExtent(
        vk::Extent3D extent,
        int mipLevel);

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

    inline void UploadToMapped(
        const void* data,
        void* mapped,
        const VkDeviceSize offset,
        const VkDeviceSize size)
    {
        std::memcpy(static_cast<char*>(mapped) + offset, data, size);
    }

    Buffer UploadToImage(
        const Context& context,
        vk::CommandBuffer commandBuffer,
        u32 queueIndex,
        const std::vector<std::span<u8>>& imageData,
        int mipLevel,
        bool loadAllMips,
        vk::Extent3D extent,
        Image& image);

    void CopyBufferToImage(
        vk::CommandBuffer commandBuffer,
        vk::Buffer buffer,
        vk::Extent3D extent,
        int maxMips,
        bool loadAllMips,
        vk::Image image);

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

    inline vk::ImageSubresourceRange GetImageSubresourceRange(
        const vk::ImageAspectFlags aspectMask,
        const u32 mipCount = 1)
    {
        const auto range = vk::ImageSubresourceRange()
                               .setAspectMask(aspectMask)
                               .setBaseArrayLayer(0)
                               .setBaseMipLevel(0)
                               .setLayerCount(1)
                               .setLevelCount(mipCount);
        return range;
    }

    inline vk::ImageSubresourceLayers GetImageSubresourceLayers(
        const vk::ImageAspectFlags aspectMask,
        const u32 mipLevel = 0)
    {
        const auto range = vk::ImageSubresourceLayers()
                               .setAspectMask(aspectMask)
                               .setBaseArrayLayer(0)
                               .setLayerCount(1)
                               .setMipLevel(mipLevel);
        return range;
    }

    vk::ImageMemoryBarrier2 ImageBarrier(
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        Image& image,
        vk::ImageAspectFlags flags,
        int mipCount = 1);

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
        const Image& image,
        const vk::ClearColorValue& clearColor);

    inline u32 PadAlignment(
        const u32 originalSize,
        const u32 alignment)
    {
        return (originalSize + alignment - 1) & ~(alignment - 1);
    }

    void* MapBuffer(
        const Context& context,
        const Buffer& buffer);
    void UnmapBuffer(
        const Context& context,
        const Buffer& buffer);
}; // namespace Swift::Vulkan::Util
