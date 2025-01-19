#include "Vulkan/VulkanUtil.hpp"
#include "SwiftStructs.hpp"
#include "Vulkan/VulkanConstants.hpp"
#include "Vulkan/VulkanInit.hpp"
#include "Vulkan/VulkanStructs.hpp"
#include "dds.hpp"

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
                [[maybe_unused]]
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

    vk::Extent3D Util::GetMipExtent(
        const vk::Extent3D extent,
        const u32 mipLevel)
    {
        u32 mipWidth = std::max(extent.width >> mipLevel, 1u);
        u32 mipHeight = std::max(extent.height >> mipLevel, 1u);
        u32 mipDepth = std::max(extent.depth >> mipLevel, 1u);
        return {mipWidth, mipHeight, mipDepth};
    }

    vk::Extent2D Util::GetMipExtent(
        const vk::Extent2D extent,
        const u32 mipLevel)
    {
        u32 mipWidth = std::max(extent.width >> mipLevel, 1u);
        u32 mipHeight = std::max(extent.height >> mipLevel, 1u);
        return {mipWidth, mipHeight};
    }

    void Util::HandleSubOptimalSwapchain(
        const u32 graphicsFamily,
        const Context& context,
        Swapchain& swapchain,
        const vk::Extent2D extent)
    {
        [[maybe_unused]]
        const auto result = context.device.waitIdle();
        VK_ASSERT(result, "Failed to wait for device while cleaning up");
        swapchain.Destroy(context);

        constexpr auto depthFormat = vk::Format::eD32Sfloat;
        const auto depthImage = Init::CreateImage(
            context,
            vk::ImageType::e2D,
            vk::Extent3D(extent, 1),
            depthFormat,
            vk::ImageUsageFlagBits::eDepthStencilAttachment,
            1,
            {},
            "Swapchain Depth");

        swapchain.SetSwapchain(Init::CreateSwapchain(context, extent, graphicsFamily))
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
        [[maybe_unused]]
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
        [[maybe_unused]]
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

    void Util::UpdateDescriptorSamplerCube(
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
                .setDstBinding(Constants::CubeSamplerBinding)
                .setDstArrayElement(arrayElement)
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setImageInfo(imageInfo);
        device.updateDescriptorSets(samplerWriteInfo, {});
    }

    vk::ImageMemoryBarrier2 Util::ImageBarrier(
        const vk::ImageLayout oldLayout,
        const vk::ImageLayout newLayout,
        Image& image,
        const vk::ImageAspectFlags flags,
        const u32 mipCount,
        const u32 arrayLayers)
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
                .setSubresourceRange(GetImageSubresourceRange(flags, mipCount, 0,arrayLayers))
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
        const Image& image,
        const vk::ClearColorValue& clearColor)
    {
        commandBuffer.clearColorImage(
            image,
            image.currentLayout,
            vk::ClearColorValue(clearColor),
            GetImageSubresourceRange(vk::ImageAspectFlagBits::eColor));
    }

    void* Util::MapBuffer(
        const Context& context,
        const Buffer& buffer)
    {
        void* mapped;
        [[maybe_unused]]
        const auto result = vmaMapMemory(context.allocator, buffer.allocation, &mapped);
        assert(result == VK_SUCCESS && "Failed to map buffer");
        return mapped;
    }

    void Util::UnmapBuffer(
        const Context& context,
        const Buffer& buffer)
    {
        vmaUnmapMemory(context.allocator, buffer.allocation);
    }

    Buffer Util::UploadToImage(
        const Context& context,
        const vk::CommandBuffer commandBuffer,
        const u32 queueIndex,
        const dds::Header& ddsImage,
        const std::string_view filePath,
        const u32 mipLevel,
        const bool loadAllMips,
        const Image& image)
    {
        u32 start;
        u32 imageSize;
        if (loadAllMips)
        {
            start = ddsImage.DataOffset();
            imageSize = ddsImage.DataSize();
        }
        else
        {
            start = ddsImage.MipOffset(mipLevel);

            if (mipLevel + 1 > ddsImage.MipLevels() - 1)
            {
                imageSize = ddsImage.DataSize() + ddsImage.DataOffset() - start;
            }
            else
            {
                imageSize = ddsImage.MipOffset(mipLevel + 1) - start;
                imageSize *= ddsImage.ArraySize();
            }
        }

        const auto buffer = Init::CreateBuffer(
            context,
            queueIndex,
            imageSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            false,
            "staging");

        std::ifstream file((filePath.data()), std::ios::binary);
        file.seekg(start, std::ios::beg);

        const auto mapped = MapBuffer(context, buffer);
        file.read(static_cast<char*>(mapped), imageSize);
        UnmapBuffer(context, buffer);

        CopyBufferToImage(commandBuffer, buffer, ddsImage, mipLevel, loadAllMips, image);
        return buffer;
    }

    void Util::CopyBufferToImage(
        const vk::CommandBuffer commandBuffer,
        const vk::Buffer buffer,
        const dds::Header& ddsImage,
        const u32 mipLevel,
        const bool loadAllMips,
        const vk::Image image)
    {
        std::vector<vk::BufferImageCopy2> copyRegions;
        if (loadAllMips)
        {
            const auto extent = ddsImage.GetVulkanExtent();
            u32 offset = 0;
            for (u32 layer = 0; layer < ddsImage.ArraySize(); layer++)
            {
                for (int i = 0; i < ddsImage.MipLevels(); i++)
                {
                    vk::Extent3D mipExtent = {
                        std::max(1u, extent.width >> i),
                        std::max(1u, extent.height >> i),
                        std::max(1u, extent.depth >> i)};
                    const auto bufferImageCopy = vk::BufferImageCopy2()
                                                     .setImageExtent(mipExtent)
                                                     .setImageSubresource(GetImageSubresourceLayers(
                                                         vk::ImageAspectFlagBits::eColor,
                                                         i,
                                                         1,
                                                         layer))
                                                     .setBufferOffset(offset);
                    copyRegions.emplace_back(bufferImageCopy);
                    offset += ddsImage.MipSize(i);
                }
            }
        }
        else
        {
            for (int i = 0; i < ddsImage.ArraySize(); i++)
            {
                const auto extent = Util::GetMipExtent(ddsImage.GetVulkanExtent(), mipLevel);
                const auto bufferImageCopy =
                    vk::BufferImageCopy2().setImageExtent(extent).setImageSubresource(
                        GetImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 1, i));
                copyRegions.emplace_back(bufferImageCopy);
            }
        }
        const auto bufferToImageInfo = vk::CopyBufferToImageInfo2()
                                           .setSrcBuffer(buffer)
                                           .setDstImage(image)
                                           .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
                                           .setRegions(copyRegions);
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
                .setSrcOffsets({
                    vk::Offset3D(),
                    vk::Offset3D(
                        static_cast<int>(srcExtent.width),
                        static_cast<int>(srcExtent.height),
                        1),
                })
                .setDstSubresource(GetImageSubresourceLayers(vk::ImageAspectFlagBits::eColor))
                .setDstOffsets({
                    vk::Offset3D(),
                    vk::Offset3D(
                        static_cast<int>(dstExtent.width),
                        static_cast<int>(dstExtent.height),
                        1),
                });
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