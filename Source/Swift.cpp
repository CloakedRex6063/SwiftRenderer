#include "Swift.hpp"
#include "Vulkan/VulkanInit.hpp"
#include "Vulkan/VulkanUtil.hpp"

#define VMA_IMPLEMENTATION
#include <Vulkan/VulkanRender.hpp>

#include "vk_mem_alloc.h"

using namespace Swift::Vulkan;

void Swift::Renderer::Init(const InitInfo& initInfo)
{
    mContext.SetInstance(Init::CreateInstance(initInfo.appName, initInfo.engineName))
        .SetSurface(Init::CreateSurface(mContext.instance, initInfo.hwnd))
        .SetGPU(Init::ChooseGPU(mContext.instance, mContext.surface))
        .SetDevice(Init::CreateDevice(mContext.gpu, mContext.surface))
        .SetAllocator(Init::CreateAllocator(mContext))
        .SetDynamicLoader(Init::CreateDynamicLoader(mContext.instance, mContext.device));

    const auto indices = Util::GetQueueFamilyIndices(mContext.gpu, mContext.surface);
    mGraphicsQueue.SetIndex(indices[0]).SetQueue(Init::GetQueue(mContext.device, indices[0]));
    mComputeQueue.SetIndex(indices[1]).SetQueue(Init::GetQueue(mContext.device, indices[1]));
    mTransferQueue.SetIndex(indices[2]).SetQueue(Init::GetQueue(mContext.device, indices[2]));

    constexpr auto renderFormat = vk::Format::eR16G16B16A16Sfloat;
    auto [renderVkImage, renderAlloc] = Init::CreateImage(
        mContext.allocator,
        Util::ToExtent3D(initInfo.extent),
        vk::ImageType::e2D,
        renderFormat,
        vk::ImageUsageFlagBits::eColorAttachment);

    const auto renderView = Init::CreateImageView(
        mContext, renderVkImage, renderFormat, vk::ImageViewType::e2D, vk::ImageAspectFlagBits::eColor, "renderImage");
    const auto renderImage =
        Image().SetFormat(renderFormat).SetImage(renderVkImage).SetAllocation(renderAlloc).SetView(renderView);

    constexpr auto depthFormat = vk::Format::eD32Sfloat;
    auto [depthVkImage, depthAlloc] = Init::CreateImage(
        mContext.allocator,
        Util::ToExtent3D(initInfo.extent),
        vk::ImageType::e2D,
        depthFormat,
        vk::ImageUsageFlagBits::eDepthStencilAttachment);

    const auto depthView = Init::CreateImageView(
        mContext, depthVkImage, depthFormat, vk::ImageViewType::e2D, vk::ImageAspectFlagBits::eDepth, "Depth Image");
    const auto depthImage =
        Image().SetFormat(depthFormat).SetImage(depthVkImage).SetAllocation(depthAlloc).SetView(depthView);

    mSwapchain.SetSwapchain(Init::CreateSwapchain(mContext, Util::ToExtent2D(initInfo.extent), mGraphicsQueue.index))
        .SetRenderImage(renderImage)
        .SetDepthImage(depthImage)
        .SetImages(Init::CreateSwapchainImages(mContext, mSwapchain));

    mFrameData.resize(mSwapchain.images.size());
    for (auto& [renderSemaphore, presentSemaphore, renderFence, commandBuffer, commandPool] : mFrameData)
    {
        renderFence = Init::CreateFence(mContext.device);
        renderSemaphore = Init::CreateSemaphore(mContext.device);
        presentSemaphore = Init::CreateSemaphore(mContext.device);
        commandPool = Init::CreateCommandPool(mContext.device, mGraphicsQueue.index);
        commandBuffer = Init::CreateCommandBuffer(mContext.device, commandPool);
    }
}
void Swift::Renderer::BeginFrame(const DynamicInfo& dynamicInfo)
{
    const auto& device = mContext.device;
    auto& [renderSemaphore, presentSemaphore, renderFence, commandBuffer, commandPool] = mFrameData[mCurrentFrame];

    auto result = device.waitForFences(renderFence, true, 1000000000);
    VK_ASSERT(result, "Failed to wait for render fence");

    const auto acquireInfo = vk::AcquireNextImageInfoKHR()
                                 .setDeviceMask(1)
                                 .setSemaphore(renderSemaphore)
                                 .setSwapchain(mSwapchain.swapchain)
                                 .setTimeout(std::numeric_limits<u64>::max());
    const auto acquireResult = device.acquireNextImage2KHR(acquireInfo);
    mResized = false;
    if (acquireResult.result != vk::Result::eSuccess)
    {
        mResized = true;
        Util::HandleSubOptimalSwapchain(
            mGraphicsQueue.index, mContext, mSwapchain, Util::ToExtent2D(dynamicInfo.extent));
        device.destroySemaphore(renderSemaphore);
        Init::CreateSemaphore(mContext.device);
        return;
    }
    result = device.resetFences(renderFence);
    VK_ASSERT(result, "Failed to reset render fence");
    mCurrentImageIndex = acquireResult.value;
}

void Swift::Renderer::Render()
{
    if (mResized)
        return;
    auto& [renderSemaphore, presentSemaphore, renderFence, commandBuffer, commandPool] = mFrameData[mCurrentFrame];
    auto result = commandBuffer.reset();
    VK_ASSERT(result, "Failed to reset command buffer");
    result = commandBuffer.begin(vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    VK_ASSERT(result, "Failed to begin command buffer");
    auto imageBarrier = vk::ImageMemoryBarrier2()
                            .setSrcAccessMask(vk::AccessFlagBits2::eMemoryWrite)
                            .setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands)
                            .setDstAccessMask(vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite)
                            .setDstStageMask(vk::PipelineStageFlagBits2::eAllCommands)
                            .setOldLayout(vk::ImageLayout::eUndefined)
                            .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
                            .setSubresourceRange(Util::GetImageSubresourceRange(vk::ImageAspectFlagBits::eColor))
                            .setImage(mSwapchain.images[mCurrentImageIndex].image);
    const auto dependencyInfo = vk::DependencyInfo().setImageMemoryBarriers(imageBarrier);
    commandBuffer.pipelineBarrier2(dependencyInfo);
    result = commandBuffer.end();
    VK_ASSERT(result, "Failed to end command buffer");
}

void Swift::Renderer::EndFrame(const DynamicInfo& dynamicInfo)
{
    if (mResized)
        return;
    auto& [renderSemaphore, presentSemaphore, renderFence, commandBuffer, commandPool] = mFrameData[mCurrentFrame];
    Util::SubmitQueue(
        mGraphicsQueue.queue,
        commandBuffer,
        renderSemaphore,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        presentSemaphore,
        vk::PipelineStageFlagBits2::eAllGraphics,
        renderFence);
    const auto result = Render::Present(mGraphicsQueue.queue, presentSemaphore, mSwapchain.swapchain, mCurrentImageIndex);
    if (result != vk::Result::eSuccess)
    {
        Util::HandleSubOptimalSwapchain(
            mGraphicsQueue.index, mContext, mSwapchain, Util::ToExtent2D(dynamicInfo.extent));
        return;
    }
    mCurrentFrame = (mCurrentFrame + 1) % mSwapchain.images.size();
}

void Swift::Renderer::Shutdown()
{
    const auto result = mContext.device.waitIdle();
    VK_ASSERT(result, "Failed to wait for device while cleaning up");
    for (const auto& frameData : mFrameData)
    {
        mContext.device.destroyFence(frameData.renderFence);
        mContext.device.destroySemaphore(frameData.renderSemaphore);
        mContext.device.destroySemaphore(frameData.presentSemaphore);
        mContext.device.destroyCommandPool(frameData.commandPool);
    }
    mSwapchain.Destroy(mContext);
    mContext.Destroy();
}