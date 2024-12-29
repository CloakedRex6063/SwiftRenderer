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
        const vk::DispatchLoaderDynamic& dynamicLoader,
        bool bUsePipeline);

    inline Image& GetSwapchainImage(Swapchain& swapchain)
    {
        return swapchain.images[swapchain.imageIndex];
    }

    inline vk::Semaphore& GetRenderSemaphore(FrameData& frameData)
    {
        return frameData.renderSemaphore;
    }

    inline vk::Semaphore& GetPresentSemaphore(FrameData& frameData)
    {
        return frameData.presentSemaphore;
    }

    inline vk::Fence& GetRenderFence(FrameData& frameData)
    {
        return frameData.renderFence;
    }

    inline vk::CommandBuffer& GetCommandBuffer(FrameData& frameData)
    {
        return frameData.renderCommand.commandBuffer;
    }

    inline vk::CommandPool& GetCommandPool(FrameData& frameData)
    {
        return frameData.renderCommand.commandPool;
    }

    inline void BeginRendering(
        const vk::CommandBuffer commandBuffer,
        const Swapchain& swapchain,
        const bool enableDepth)
    {
        const auto colorAttachment = vk::RenderingAttachmentInfo()
                                         .setImageView(swapchain.renderImage.imageView)
                                         .setClearValue(vk::ClearColorValue().setFloat32({0.f}))
                                         .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                                         .setLoadOp(vk::AttachmentLoadOp::eLoad)
                                         .setStoreOp(vk::AttachmentStoreOp::eStore);
        const auto depthAttachment = vk::RenderingAttachmentInfo()
                                         .setImageView(swapchain.depthImage.imageView)
                                         .setClearValue(vk::ClearColorValue().setFloat32({0.f}))
                                         .setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal)
                                         .setLoadOp(vk::AttachmentLoadOp::eClear)
                                         .setStoreOp(vk::AttachmentStoreOp::eStore);
        const auto renderingInfo =
            vk::RenderingInfo()
                .setColorAttachments(colorAttachment)
                .setPDepthAttachment(enableDepth ? &depthAttachment : nullptr)
                .setLayerCount(1)
                .setRenderArea(vk::Rect2D().setExtent(swapchain.extent));
        commandBuffer.beginRendering(renderingInfo);
    }

} // namespace Swift::Vulkan::Render
