#pragma once
#include "VulkanStructs.hpp"

namespace Swift::Vulkan::Render
{
    u32 AcquireNextImage(
        const Queue& queue,
        const Context& context,
        Swapchain& swapchain,
        vk::Semaphore semaphore,
        vk::Extent2D extent);

    void Present(
        const Context& context,
        Swapchain& swapchain,
        Queue queue,
        vk::Semaphore semaphore,
        vk::Extent2D extent);

    void DefaultRenderConfig(
        vk::CommandBuffer commandBuffer,
        vk::Extent2D extent,
        const vk::DispatchLoaderDynamic& dynamicLoader);

    inline Image& GetSwapchainImage(Swapchain& swapchain)
    {
        return swapchain.images[swapchain.imageIndex];
    }

    inline vk::Semaphore& GetRenderSemaphore(
        const std::span<FrameData> frameData,
        const u32 frameIndex)
    {
        return frameData[frameIndex].renderSemaphore;
    }

    inline vk::Semaphore& GetPresentSemaphore(
        const std::span<FrameData> frameData,
        const u32 frameIndex)
    {
        return frameData[frameIndex].presentSemaphore;
    }

    inline vk::Fence& GetRenderFence(
        const std::span<FrameData> frameData,
        const u32 frameIndex)
    {
        return frameData[frameIndex].renderFence;
    }

    inline vk::CommandBuffer& GetCommandBuffer(
        const std::span<FrameData> frameData,
        const u32 frameIndex)
    {
        return frameData[frameIndex].renderCommand.commandBuffer;
    }

    inline vk::CommandPool& GetCommandPool(
        const std::span<FrameData>& frameData,
        const u32 frameIndex)
    {
        return frameData[frameIndex].renderCommand.commandPool;
    }

    inline void BeginRendering(
        const vk::CommandBuffer commandBuffer,
        const Swapchain& swapchain)
    {
        const auto colorAttachment = vk::RenderingAttachmentInfo()
                                         .setImageView(swapchain.renderImage.imageView)
                                         .setClearValue(vk::ClearColorValue({0, 0, 0, 0}))
                                         .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                                         .setLoadOp(vk::AttachmentLoadOp::eLoad)
                                         .setStoreOp(vk::AttachmentStoreOp::eStore);
        const auto depthAttachment = vk::RenderingAttachmentInfo()
                                         .setImageView(swapchain.depthImage.imageView)
                                         .setClearValue(vk::ClearColorValue({0, 0, 0, 0}))
                                         .setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal)
                                         .setLoadOp(vk::AttachmentLoadOp::eClear)
                                         .setStoreOp(vk::AttachmentStoreOp::eStore);
        const auto renderingInfo = vk::RenderingInfo()
                                       .setColorAttachments(colorAttachment)
                                       .setPDepthAttachment(&depthAttachment)
                                       .setLayerCount(1)
                                       .setRenderArea(vk::Rect2D().setExtent(swapchain.extent));
        commandBuffer.beginRendering(renderingInfo);
    }

} // namespace Swift::Vulkan::Render
