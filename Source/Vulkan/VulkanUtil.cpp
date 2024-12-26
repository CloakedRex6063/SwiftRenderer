#include "Vulkan/VulkanUtil.hpp"
#include "Vulkan/VulkanInit.hpp"
#include "Vulkan/VulkanStructs.hpp"

#include <Vulkan/VulkanConstants.hpp>
#include <set>

namespace Swift::Vulkan
{
    std::vector<u32> Util::GetQueueFamilyIndices(
        const vk::PhysicalDevice physicalDevice,
        const vk::SurfaceKHR surface)
    {
        std::vector<u32> indices;
        std::optional<u32> graphicsFamily;
        std::optional<u32> computeFamily;
        std::optional<u32> transferFamily;
        const auto queueFamilies = physicalDevice.getQueueFamilyProperties();
        for (const auto [index, family] : std::views::enumerate(queueFamilies))
        {
            if (family.queueFlags & vk::QueueFlagBits::eGraphics && !graphicsFamily.has_value())
            {
                const auto [result, support] = physicalDevice.getSurfaceSupportKHR(index, surface);
                VK_ASSERT(result, "Failed to get surface for queue family");
                graphicsFamily = static_cast<u32>(index);
                indices.emplace_back(static_cast<u32>(index));
                continue;
            }

            if (family.queueFlags & vk::QueueFlagBits::eCompute && !computeFamily.has_value())
            {
                computeFamily = static_cast<u32>(index);
                indices.emplace_back(static_cast<u32>(index));
                continue;
            }

            if (family.queueFlags & vk::QueueFlagBits::eTransfer && !transferFamily.has_value())
            {
                transferFamily = static_cast<u32>(index);
                indices.emplace_back(static_cast<u32>(index));
            }
        }
        return indices;
    }

    void Util::HandleSubOptimalSwapchain(
        const u32 graphicsFamily,
        const Context& context,
        Swapchain& swapchain,
        const vk::Extent2D extent)
    {
        const auto result = context.device.waitIdle();
        VK_ASSERT(result, "Failed to wait for device while cleaning up");
        swapchain.Destroy(context);
        constexpr auto renderFormat = vk::Format::eR16G16B16A16Sfloat;
        auto [renderVkImage, renderAlloc] = Init::CreateImage(
            context.allocator,
            vk::Extent3D(extent, 1),
            vk::ImageType::e2D,
            renderFormat,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc |
                vk::ImageUsageFlagBits::eTransferDst);

        const auto renderView = Init::CreateImageView(
            context,
            renderVkImage,
            renderFormat,
            vk::ImageViewType::e2D,
            vk::ImageAspectFlagBits::eColor,
            "renderImage");
        const auto renderImage = Image()
                                     .SetFormat(renderFormat)
                                     .SetImage(renderVkImage)
                                     .SetAllocation(renderAlloc)
                                     .SetView(renderView);

        constexpr auto depthFormat = vk::Format::eD32Sfloat;
        auto [depthVkImage, depthAlloc] = Init::CreateImage(
            context.allocator,
            vk::Extent3D(extent, 1),
            vk::ImageType::e2D,
            depthFormat,
            vk::ImageUsageFlagBits::eDepthStencilAttachment);

        const auto depthView = Init::CreateImageView(
            context,
            depthVkImage,
            depthFormat,
            vk::ImageViewType::e2D,
            vk::ImageAspectFlagBits::eDepth,
            "Depth Image");
        const auto depthImage = Image()
                                    .SetFormat(depthFormat)
                                    .SetImage(depthVkImage)
                                    .SetAllocation(depthAlloc)
                                    .SetView(depthView);
        swapchain.SetSwapchain(Init::CreateSwapchain(context, extent, graphicsFamily))
            .SetRenderImage(renderImage)
            .SetDepthImage(depthImage)
            .SetImages(Init::CreateSwapchainImages(context, swapchain))
            .SetExtent(extent);
    }

    void Util::SubmitQueue(
        const vk::Queue queue,
        const vk::CommandBuffer commandBuffer,
        const vk::Semaphore waitSemaphore,
        const vk::PipelineStageFlags2 waitStageMask,
        const vk::Semaphore signalSemaphore,
        const vk::PipelineStageFlags2 signalStageMask,
        const vk::Fence fence)
    {
        auto commandInfo =
            vk::CommandBufferSubmitInfo().setDeviceMask(1).setCommandBuffer(commandBuffer);
        auto waitInfo =
            vk::SemaphoreSubmitInfo().setSemaphore(waitSemaphore).setStageMask(waitStageMask);
        auto signalInfo =
            vk::SemaphoreSubmitInfo().setSemaphore(signalSemaphore).setStageMask(signalStageMask);
        const auto submitInfo = vk::SubmitInfo2()
                                    .setCommandBufferInfos(commandInfo)
                                    .setWaitSemaphoreInfos(waitInfo)
                                    .setSignalSemaphoreInfos(signalInfo);
        const auto result = queue.submit2(submitInfo, fence);
        VK_ASSERT(result, "Failed to submit queue");
    }

    void Util::SubmitQueueHost(
        const vk::Queue queue,
        const vk::CommandBuffer commandBuffer,
        const vk::Fence fence)
    {
        auto commandInfo =
            vk::CommandBufferSubmitInfo().setDeviceMask(1).setCommandBuffer(commandBuffer);
        const auto submitInfo = vk::SubmitInfo2().setCommandBufferInfos(commandInfo);
        const auto result = queue.submit2(submitInfo, fence);
        VK_ASSERT(result, "Failed to submit queue");
    }

    void Util::UpdateDescriptorImage(
        const vk::DescriptorSet set,
        const vk::ImageView imageView,
        const vk::Sampler sampler,
        const u32 arrayElement,
        const vk::Device& device)
    {
        const auto imageInfo = vk::DescriptorImageInfo()
                                   .setImageLayout(vk::ImageLayout::eGeneral)
                                   .setImageView(imageView)
                                   .setSampler(sampler);
        const auto imageWriteInfo = vk::WriteDescriptorSet()
                                        .setDstSet(set)
                                        .setDstBinding(Constants::ImageBinding)
                                        .setDstArrayElement(arrayElement)
                                        .setDescriptorCount(1)
                                        .setDescriptorType(vk::DescriptorType::eStorageImage)
                                        .setImageInfo(imageInfo);
        device.updateDescriptorSets(imageWriteInfo, {});
    }

    void Util::UpdateDescriptorSampler(
    const vk::DescriptorSet set,
    const vk::ImageView imageView,
    const vk::Sampler sampler,
    const u32 arrayElement,
    const vk::Device& device)
    {
        const auto imageInfo = vk::DescriptorImageInfo()
                                   .setImageLayout(vk::ImageLayout::eGeneral)
                                   .setImageView(imageView)
                                   .setSampler(sampler);
        const auto samplerWriteInfo =
            vk::WriteDescriptorSet()
                .setDstSet(set)
                .setDstBinding(Constants::SamplerBinding)
                .setDstArrayElement(arrayElement)
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setImageInfo(imageInfo);
        device.updateDescriptorSets(samplerWriteInfo, {});
    }

    vk::ImageSubresourceRange Util::GetImageSubresourceRange(
        const vk::ImageAspectFlags aspectMask,
        const u32 mipCount)
    {
        const auto range = vk::ImageSubresourceRange()
                               .setAspectMask(aspectMask)
                               .setBaseArrayLayer(0)
                               .setBaseMipLevel(0)
                               .setLayerCount(1)
                               .setLevelCount(mipCount);
        return range;
    }

    vk::ImageSubresourceLayers
    Util::GetImageSubresourceLayers(const vk::ImageAspectFlags aspectMask)
    {
        const auto range = vk::ImageSubresourceLayers()
                               .setAspectMask(aspectMask)
                               .setBaseArrayLayer(0)
                               .setLayerCount(1)
                               .setMipLevel(0);
        return range;
    }

    vk::ImageMemoryBarrier2 Util::ImageBarrier(
        const vk::ImageLayout oldLayout,
        const vk::ImageLayout newLayout,
        Image& image,
        const vk::ImageAspectFlags flags)
    {
        const auto imageBarrier =
            vk::ImageMemoryBarrier2()
                .setSrcAccessMask(vk::AccessFlagBits2::eMemoryWrite)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands)
                .setDstAccessMask(
                    vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite)
                .setDstStageMask(vk::PipelineStageFlagBits2::eAllCommands)
                .setOldLayout(oldLayout)
                .setNewLayout(newLayout)
                .setSubresourceRange(GetImageSubresourceRange(flags))
                .setImage(image);
        image.currentLayout = newLayout;
        return imageBarrier;
    }

    void Util::PipelineBarrier(
        const vk::CommandBuffer commandBuffer,
        vk::ArrayProxy<vk::ImageMemoryBarrier2> imageBarriers)
    {
        const auto dependency = vk::DependencyInfo().setImageMemoryBarriers(imageBarriers);
        commandBuffer.pipelineBarrier2(dependency);
    }

    void Util::ClearColorImage(
        const vk::CommandBuffer& commandBuffer,
        const Image& image)
    {
        commandBuffer.clearColorImage(
            image,
            image.currentLayout,
            vk::ClearColorValue({0, 0, 0, 0}),
            GetImageSubresourceRange(vk::ImageAspectFlagBits::eColor));
    }

    Buffer Util::UploadToImage(
        const Context& context,
        const vk::CommandBuffer commandBuffer,
        const u32 queueIndex,
        const std::span<u8> imageData,
        const vk::Extent3D extent,
        Image& image)
    {
        const auto imageBarrier = ImageBarrier(
            image.currentLayout,
            vk::ImageLayout::eTransferDstOptimal,
            image,
            vk::ImageAspectFlagBits::eColor);
        PipelineBarrier(commandBuffer, imageBarrier);
        const auto imageSize = imageData.size() * sizeof(u8);
        auto [staging, alloc, allocInfo] = Init::CreateBuffer(
            context,
            queueIndex,
            imageSize,
            vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst);
        const auto buffer =
            Buffer().SetBuffer(staging).SetAllocation(alloc).SetAllocationInfo(allocInfo);
        UploadToBuffer(context, imageData.data(), buffer, 0, imageSize);
        CopyBufferToImage(commandBuffer, staging, extent, image);
        const auto dstImageBarrier = ImageBarrier(
            image.currentLayout,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            image,
            vk::ImageAspectFlagBits::eColor);
        PipelineBarrier(commandBuffer, dstImageBarrier);
        return buffer;
    }

    void Util::CopyBufferToImage(
        const vk::CommandBuffer commandBuffer,
        const vk::Buffer buffer,
        const vk::Extent3D extent,
        const vk::Image image)
    {

        const auto bufferImageCopy =
            vk::BufferImageCopy2().setImageExtent(extent).setImageSubresource(
                GetImageSubresourceLayers(vk::ImageAspectFlagBits::eColor));
        const auto bufferToImageInfo = vk::CopyBufferToImageInfo2()
                                           .setSrcBuffer(buffer)
                                           .setDstImage(image)
                                           .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
                                           .setRegions(bufferImageCopy);
        commandBuffer.copyBufferToImage2(bufferToImageInfo);
    }

    void Util::CopyImage(
        const vk::CommandBuffer commandBuffer,
        const vk::Image srcImage,
        const vk::ImageLayout srcLayout,
        const vk::Image dstImage,
        const vk::ImageLayout dstLayout,
        const vk::Extent2D extent)
    {
        auto copyRegion =
            vk::ImageCopy2()
                .setSrcOffset({0, 0, 0})
                .setDstOffset({0, 0, 0})
                .setSrcSubresource(GetImageSubresourceLayers(vk::ImageAspectFlagBits::eColor))
                .setDstSubresource(GetImageSubresourceLayers(vk::ImageAspectFlagBits::eColor))
                .setExtent(vk::Extent3D(extent, 1));
        const auto copyImageInfo = vk::CopyImageInfo2()
                                       .setSrcImage(srcImage)
                                       .setSrcImageLayout(srcLayout)
                                       .setDstImage(dstImage)
                                       .setDstImageLayout(dstLayout)
                                       .setRegions(copyRegion);
        commandBuffer.copyImage2(copyImageInfo);
    }

    void Util::BlitImage(
        const vk::CommandBuffer commandBuffer,
        const vk::Image srcImage,
        const vk::ImageLayout srcLayout,
        const vk::Image dstImage,
        const vk::ImageLayout dstLayout,
        const vk::Extent2D srcExtent,
        const vk::Extent2D dstExtent)
    {
        const auto blitRegion =
            vk::ImageBlit2()
                .setSrcSubresource(GetImageSubresourceLayers(vk::ImageAspectFlagBits::eColor))
                .setSrcOffsets(
                    {vk::Offset3D(0, 0, 0), vk::Offset3D(srcExtent.width, srcExtent.height, 1)})
                .setDstSubresource(GetImageSubresourceLayers(vk::ImageAspectFlagBits::eColor))
                .setDstOffsets(
                    {vk::Offset3D(0, 0, 0), vk::Offset3D(dstExtent.width, dstExtent.height, 1)});
        const auto blitInfo = vk::BlitImageInfo2()
                                  .setSrcImage(srcImage)
                                  .setSrcImageLayout(srcLayout)
                                  .setDstImage(dstImage)
                                  .setDstImageLayout(dstLayout)
                                  .setRegions(blitRegion)
                                  .setFilter(vk::Filter::eLinear);
        commandBuffer.blitImage2(blitInfo);
    }
} // namespace Swift::Vulkan