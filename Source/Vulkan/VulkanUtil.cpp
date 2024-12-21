#include "Vulkan/VulkanUtil.hpp"
#include "Vulkan/VulkanInit.hpp"
#include "Vulkan/VulkanStructs.hpp"

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

    void Util::WaitFence(
        const vk::Device device,
        const vk::Fence fence,
        const u64 timeout)
    {
        const auto result = device.waitForFences(fence, true, timeout);
        VK_ASSERT(result, "Failed to wait fences");
    }

    void Util::HandleSubOptimalSwapchain(
        const u32 graphicsFamily,
        const Context& context,
        Swapchain& swapchain,
        const vk::Extent2D extent)
    {
        swapchain.Destroy(context);
        constexpr auto renderFormat = vk::Format::eR16G16B16A16Sfloat;
        auto [renderVkImage, renderAlloc] = Init::CreateImage(
            context.allocator,
            vk::Extent3D(extent, 1),
            vk::ImageType::e2D,
            renderFormat,
            vk::ImageUsageFlagBits::eColorAttachment);

        const auto renderView = Init::CreateImageView(
            context,
            renderVkImage,
            renderFormat,
            vk::ImageViewType::e2D,
            vk::ImageAspectFlagBits::eColor,
            "renderImage");
        const auto renderImage =
            Image().SetFormat(renderFormat).SetImage(renderVkImage).SetAllocation(renderAlloc).SetView(renderView);

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
        const auto depthImage =
            Image().SetFormat(depthFormat).SetImage(depthVkImage).SetAllocation(depthAlloc).SetView(depthView);
        swapchain.SetSwapchain(Init::CreateSwapchain(context, extent, graphicsFamily))
            .SetRenderImage(renderImage)
            .SetDepthImage(depthImage)
            .SetImages(Init::CreateSwapchainImages(context, swapchain));
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
        auto commandInfo = vk::CommandBufferSubmitInfo().setDeviceMask(1).setCommandBuffer(commandBuffer);
        auto waitInfo = vk::SemaphoreSubmitInfo().setSemaphore(waitSemaphore).setStageMask(waitStageMask);
        auto signalInfo = vk::SemaphoreSubmitInfo().setSemaphore(signalSemaphore).setStageMask(signalStageMask);
        const auto submitInfo = vk::SubmitInfo2()
                                    .setCommandBufferInfos(commandInfo)
                                    .setWaitSemaphoreInfos(waitInfo)
                                    .setSignalSemaphoreInfos(signalInfo);
        const auto result = queue.submit2(submitInfo, fence);
        VK_ASSERT(result, "Failed to submit queue");
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
}  // namespace Swift::Vulkan