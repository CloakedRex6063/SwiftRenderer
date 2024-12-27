#include "Swift.hpp"
#include "Utils/FileIO.hpp"
#include "Vulkan/VulkanInit.hpp"
#include "Vulkan/VulkanRender.hpp"
#include "Vulkan/VulkanStructs.hpp"
#include "Vulkan/VulkanUtil.hpp"
#include "dds.hpp"

#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

namespace
{
    using namespace Swift;
    Vulkan::Context gContext;
    Vulkan::Queue gGraphicsQueue;
    Vulkan::Queue gComputeQueue;
    Vulkan::Queue gTransferQueue;
    Vulkan::Command gTransferCommand;
    vk::Fence gTransferFence;

    Vulkan::Swapchain gSwapchain;
    Vulkan::BindlessDescriptor gDescriptor;
    // TODO: sampler pool for all types of samplers
    vk::Sampler gLinearSampler;

    std::vector<Vulkan::Buffer> gBuffers;
    std::vector<Vulkan::Shader> gShaders;
    u32 gCurrentShader = 0;
    std::vector<Vulkan::Image> gWriteableImages;
    std::vector<Vulkan::Image> gSamplerImages;
    std::vector<Vulkan::FrameData> gFrameData;
    u32 gCurrentFrame = 0;
} // namespace

using namespace Vulkan;

void Swift::Init(const InitInfo& initInfo)
{
    gContext.SetInstance(Init::CreateInstance(initInfo.appName, initInfo.engineName))
        .SetSurface(Init::CreateSurface(gContext.instance, initInfo.hwnd))
        .SetGPU(Init::ChooseGPU(gContext.instance, gContext.surface))
        .SetDevice(Init::CreateDevice(gContext.gpu, gContext.surface))
        .SetAllocator(Init::CreateAllocator(gContext))
        .SetDynamicLoader(Init::CreateDynamicLoader(gContext.instance, gContext.device));

    const auto indices = Util::GetQueueFamilyIndices(gContext.gpu, gContext.surface);
    const auto graphicsQueue = Init::GetQueue(gContext, indices[0], "Graphics Queue");
    gGraphicsQueue.SetIndex(indices[0]).SetQueue(graphicsQueue);
    const auto computeQueue = Init::GetQueue(gContext, indices[1], "Compute Queue");
    gComputeQueue.SetIndex(indices[1]).SetQueue(computeQueue);
    const auto transferQueue = Init::GetQueue(gContext, indices[2], "Transfer Queue");
    gTransferQueue.SetIndex(indices[2]).SetQueue(transferQueue);

    constexpr auto renderFormat = vk::Format::eR16G16B16A16Sfloat;
    auto [renderVkImage, renderAlloc] = Init::CreateImage(
        gContext.allocator,
        Util::To3D(initInfo.extent),
        vk::ImageType::e2D,
        renderFormat,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc |
            vk::ImageUsageFlagBits::eTransferDst);
    const auto renderView =
        Init::CreateColorImageView(gContext, renderVkImage, renderFormat, "Render Image");
    const auto renderImage = Image()
                                 .SetFormat(renderFormat)
                                 .SetImage(renderVkImage)
                                 .SetAllocation(renderAlloc)
                                 .SetView(renderView);

    constexpr auto depthFormat = vk::Format::eD32Sfloat;
    auto [depthVkImage, depthAlloc] = Init::CreateImage(
        gContext.allocator,
        Util::To3D(initInfo.extent),
        vk::ImageType::e2D,
        depthFormat,
        vk::ImageUsageFlagBits::eDepthStencilAttachment);
    const auto depthView = Init::CreateDepthImageView(gContext, depthVkImage, "Depth Image");
    const auto depthImage = Image()
                                .SetFormat(depthFormat)
                                .SetImage(depthVkImage)
                                .SetAllocation(depthAlloc)
                                .SetView(depthView);

    const auto swapchain =
        Init::CreateSwapchain(gContext, Util::To2D(initInfo.extent), gGraphicsQueue.index);
    gSwapchain.SetSwapchain(swapchain)
        .SetRenderImage(renderImage)
        .SetDepthImage(depthImage)
        .SetImages(Init::CreateSwapchainImages(gContext, gSwapchain))
        .SetExtent(Util::To2D(initInfo.extent));

    gFrameData.resize(gSwapchain.images.size());
    for (auto& [renderSemaphore, presentSemaphore, renderFence, renderCommand] : gFrameData)
    {
        renderFence =
            Init::CreateFence(gContext, vk::FenceCreateFlagBits::eSignaled, "Render Fence");
        renderSemaphore = Init::CreateSemaphore(gContext, "Render Semaphore");
        presentSemaphore = Init::CreateSemaphore(gContext, "Present Semaphore");
        renderCommand.commandPool =
            Init::CreateCommandPool(gContext, gGraphicsQueue.index, "Command Pool");
        renderCommand.commandBuffer =
            Init::CreateCommandBuffer(gContext, renderCommand.commandPool, "Command Buffer");
    }

    gDescriptor.SetDescriptorSetLayout(Init::CreateDescriptorSetLayout(gContext))
        .SetDescriptorPool(Init::CreateDescriptorPool(gContext))
        .SetDescriptorSet(
            Init::CreateDescriptorSet(gContext, gDescriptor.pool, gDescriptor.setLayout));

    gLinearSampler = Init::CreateSampler(gContext);

    gTransferCommand.commandPool =
        Init::CreateCommandPool(gContext, gTransferQueue.index, "Transfer Command Pool");
    gTransferCommand.commandBuffer = Init::CreateCommandBuffer(
        gContext,
        gTransferCommand.commandPool,
        "Transfer Command Buffer");
    gTransferFence = Init::CreateFence(gContext, {}, "Transfer Fence");
}

void Swift::Shutdown()
{
    const auto result = gContext.device.waitIdle();
    VK_ASSERT(result, "Failed to wait for device while cleaning up");
    for (const auto& frameData : gFrameData)
    {
        frameData.Destroy(gContext);
    }

    gDescriptor.Destroy(gContext);

    for (const auto& [shaders, stageFlags, pipelineLayout] : gShaders)
    {
        for (const auto& shader : shaders)
        {
            gContext.device.destroy(shader, nullptr, gContext.dynamicLoader);
        }
        gContext.device.destroy(pipelineLayout);
    }

    for (auto& image : gWriteableImages)
    {
        image.Destroy(gContext);
    }

    for (auto& image : gSamplerImages)
    {
        image.Destroy(gContext);
    }

    for (auto& buffer : gBuffers)
    {
        buffer.Destroy(gContext);
    }

    gTransferCommand.Destroy(gContext);
    gContext.device.destroy(gTransferFence);

    gContext.device.destroy(gLinearSampler);

    gSwapchain.Destroy(gContext);
    gContext.Destroy();
}

void Swift::BeginFrame(const DynamicInfo& dynamicInfo)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gFrameData, gCurrentFrame);
    const auto& renderSemaphore = Render::GetRenderSemaphore(gFrameData, gCurrentFrame);
    const auto& renderFence = Render::GetRenderFence(gFrameData, gCurrentFrame);

    Util::WaitFence(gContext, renderFence, 1000000000);

    gSwapchain.imageIndex = Render::AcquireNextImage(
        gGraphicsQueue,
        gContext,
        gSwapchain,
        renderSemaphore,
        Util::To2D(dynamicInfo.extent));
    Util::ResetFence(gContext, renderFence);
    Util::BeginOneTimeCommand(commandBuffer);
}

void Swift::EndFrame(const DynamicInfo& dynamicInfo)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gFrameData, gCurrentFrame);
    const auto& renderSemaphore = Render::GetRenderSemaphore(gFrameData, gCurrentFrame);
    const auto& presentSemaphore = Render::GetPresentSemaphore(gFrameData, gCurrentFrame);
    const auto& renderFence = Render::GetRenderFence(gFrameData, gCurrentFrame);
    auto& swapchainImage = Render::GetSwapchainImage(gSwapchain);

    const auto copyRenderBarrier = Util::ImageBarrier(
        gSwapchain.renderImage.currentLayout,
        vk::ImageLayout::eTransferSrcOptimal,
        gSwapchain.renderImage,
        vk::ImageAspectFlagBits::eColor);

    const auto copySwapchainBarrier = Util::ImageBarrier(
        swapchainImage.currentLayout,
        vk::ImageLayout::eTransferDstOptimal,
        swapchainImage,
        vk::ImageAspectFlagBits::eColor);

    Util::PipelineBarrier(commandBuffer, {copyRenderBarrier, copySwapchainBarrier});

    Util::BlitImage(
        commandBuffer,
        gSwapchain.renderImage,
        gSwapchain.renderImage.currentLayout,
        swapchainImage,
        swapchainImage.currentLayout,
        gSwapchain.extent,
        gSwapchain.extent);

    const auto presentBarrier = Util::ImageBarrier(
        swapchainImage.currentLayout,
        vk::ImageLayout::ePresentSrcKHR,
        swapchainImage,
        vk::ImageAspectFlagBits::eColor);
    Util::PipelineBarrier(commandBuffer, presentBarrier);

    Util::EndCommand(commandBuffer);
    Util::SubmitQueue(
        gGraphicsQueue,
        commandBuffer,
        renderSemaphore,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        presentSemaphore,
        vk::PipelineStageFlagBits2::eAllGraphics,
        renderFence);
    Render::Present(
        gContext,
        gSwapchain,
        gGraphicsQueue,
        presentSemaphore,
        Util::To2D(dynamicInfo.extent));

    gCurrentFrame = (gCurrentFrame + 1) % gSwapchain.images.size();
}

void Swift::BeginRendering()
{
    const auto& commandBuffer = Render::GetCommandBuffer(gFrameData, gCurrentFrame);
    Render::BeginRendering(commandBuffer, gSwapchain);
    Render::DefaultRenderConfig(commandBuffer, gSwapchain.extent, gContext.dynamicLoader);
}

void Swift::EndRendering()
{
    const auto& commandBuffer = Render::GetCommandBuffer(gFrameData, gCurrentFrame);
    commandBuffer.endRendering();
}

ShaderObject Swift::CreateGraphicsShaderObject(
    const std::string_view vertexPath,
    const std::string_view fragmentPath,
    const u32 pushConstantSize)
{
    const auto pushConstantRange = vk::PushConstantRange()
                                       .setSize(pushConstantSize)
                                       .setStageFlags(vk::ShaderStageFlagBits::eAllGraphics);

    auto vertexCode = FileIO::ReadBinaryFile(vertexPath);
    auto fragmentCode = FileIO::ReadBinaryFile(fragmentPath);
    const std::array shaderCreateInfo{
        Init::CreateShader(
            vertexCode,
            vk::ShaderStageFlagBits::eVertex,
            gDescriptor.setLayout,
            pushConstantRange,
            vk::ShaderCreateFlagBitsEXT::eLinkStage,
            vk::ShaderStageFlagBits::eFragment),
        Init::CreateShader(
            fragmentCode,
            vk::ShaderStageFlagBits::eFragment,
            gDescriptor.setLayout,
            pushConstantRange,
            vk::ShaderCreateFlagBitsEXT::eLinkStage),
    };

    auto [result, shaders] =
        gContext.device.createShadersEXT(shaderCreateInfo, nullptr, gContext.dynamicLoader);
    VK_ASSERT(result, "Failed to create shaders!");

    auto layoutCreateInfo = vk::PipelineLayoutCreateInfo().setSetLayouts(gDescriptor.setLayout);

    if (pushConstantSize > 0)
    {
        layoutCreateInfo.setPushConstantRanges(pushConstantRange);
    }

    // TODO: cache similar pipeline layouts
    const auto [pipeResult, pipelineLayout] =
        gContext.device.createPipelineLayout(layoutCreateInfo);
    VK_ASSERT(pipeResult, "Failed to create pipeline layout for shader!");

    constexpr auto stageFlags = {
        vk::ShaderStageFlagBits::eVertex,
        vk::ShaderStageFlagBits::eFragment};
    gShaders.emplace_back(
        Shader().SetShaders(shaders).SetPipelineLayout(pipelineLayout).SetStageFlags(stageFlags));

    const auto index = static_cast<u32>(gShaders.size() - 1);
    const auto name = "Graphics Layout " + std::to_string(index);
    Util::NameObject(pipelineLayout, name, gContext);
    return ShaderObject().SetIndex(index);
}

ShaderObject Swift::CreateComputeShaderObject(
    const std::string& computePath,
    const u32 pushConstantSize)
{
    auto computeCode = FileIO::ReadBinaryFile(computePath);

    const auto pushConstantRange = vk::PushConstantRange()
                                       .setSize(pushConstantSize)
                                       .setStageFlags(vk::ShaderStageFlagBits::eCompute);

    const auto shaderCreateInfo = Init::CreateShader(
        computeCode,
        vk::ShaderStageFlagBits::eCompute,
        gDescriptor.setLayout,
        pushConstantRange);
    auto [result, shader] =
        gContext.device.createShaderEXT(shaderCreateInfo, nullptr, gContext.dynamicLoader);
    VK_ASSERT(result, "Failed to create shaders!");

    auto layoutCreateInfo = vk::PipelineLayoutCreateInfo().setSetLayouts(gDescriptor.setLayout);

    if (pushConstantSize > 0)
    {
        layoutCreateInfo.setPushConstantRanges(pushConstantRange);
    }

    const auto [pipeResult, pipelineLayout] =
        gContext.device.createPipelineLayout(layoutCreateInfo);
    VK_ASSERT(pipeResult, "Failed to create pipeline layout for shader!");

    gShaders.emplace_back(
        Shader()
            .SetShaders({shader})
            .SetPipelineLayout(pipelineLayout)
            .SetStageFlags({vk::ShaderStageFlagBits::eCompute}));
    const auto index = static_cast<u32>(gShaders.size() - 1);
    const auto name = "Compute Layout " + std::to_string(index);
    Util::NameObject(pipelineLayout, name, gContext);
    return ShaderObject().SetIndex(index);
}

void Swift::BindShader(const ShaderObject& shaderObject)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gFrameData, gCurrentFrame);
    const auto& [shaders, stageFlags, pipelineLayout] = gShaders.at(shaderObject.index);
    commandBuffer.bindShadersEXT(stageFlags, shaders, gContext.dynamicLoader);
    const auto it = std::ranges::find(stageFlags, vk::ShaderStageFlagBits::eCompute);
    auto pipelineBindPoint = vk::PipelineBindPoint::eCompute;
    if (it == std::end(stageFlags))
    {
        pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    }
    commandBuffer.bindDescriptorSets(pipelineBindPoint, pipelineLayout, 0, gDescriptor.set, {});

    gCurrentShader = shaderObject.index;
}

void Swift::Draw(
    const u32 vertexCount,
    const u32 instanceCount,
    const u32 firstVertex,
    const u32 firstInstance)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gFrameData, gCurrentFrame);
    commandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void Swift::DrawIndexed(
    const u32 indexCount,
    const u32 instanceCount,
    const u32 firstIndex,
    const int vertexOffset,
    const u32 firstInstance)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gFrameData, gCurrentFrame);
    commandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

ImageObject Swift::CreateWriteableImage(glm::uvec2 size)
{
    constexpr auto imageUsage = vk::ImageUsageFlagBits::eColorAttachment |
                                vk::ImageUsageFlagBits::eTransferSrc |
                                vk::ImageUsageFlagBits::eTransferDst |
                                vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;

    constexpr auto format = vk::Format::eR16G16B16A16Sfloat;
    auto [vkImage, allocation] = Init::CreateImage(
        gContext.allocator,
        Util::To3D(size),
        vk::ImageType::e2D,
        format,
        imageUsage);

    const auto view = Init::CreateColorImageView(gContext, vkImage, format, "Render Image");
    const auto renderImage =
        Image().SetFormat(format).SetImage(vkImage).SetAllocation(allocation).SetView(view);

    gWriteableImages.emplace_back(renderImage);

    const auto arrayElement = static_cast<u32>(gWriteableImages.size() - 1);
    Util::UpdateDescriptorImage(
        gDescriptor.set,
        renderImage.imageView,
        gLinearSampler,
        arrayElement,
        gContext);

    return ImageObject().SetExtent(size).SetIndex(static_cast<u32>(gWriteableImages.size() - 1));
}

ImageObject Swift::CreateImageFromFile(
    const std::filesystem::path& filePath,
    int mipLevel,
    bool loadAllMipMaps)
{
    vk::Extent3D extent;
    if (filePath.extension() == ".dds")
    {
        dds::Image ddsImage;
        const auto readResult = dds::readFile(filePath, &ddsImage);
        assert(readResult == dds::Success);

        auto imageCreateInfo = dds::getVulkanImageCreateInfo(&ddsImage);
        auto imageViewCreateInfo = dds::getVulkanImageViewCreateInfo(&ddsImage);

        imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        if (!loadAllMipMaps)
        {
            imageCreateInfo.mipLevels = 1;
            imageCreateInfo.extent = Util::GetMipExtent(imageCreateInfo.extent, mipLevel);
        }

        auto [vulkanImage, alloc] = Init::CreateImage(gContext, imageCreateInfo);
        Util::NameObject(vulkanImage, "Image", gContext);
        imageViewCreateInfo.image = vulkanImage;
        if (!loadAllMipMaps)
        {
            imageViewCreateInfo.subresourceRange.levelCount = 1;
        }
        const auto imageView = Init::CreateImageView(gContext, imageViewCreateInfo);

        auto image = Image()
                         .SetImage(vulkanImage)
                         .SetAllocation(alloc)
                         .SetView(imageView)
                         .SetFormat(static_cast<vk::Format>(imageViewCreateInfo.format));
        gSamplerImages.emplace_back(image);

        // TODO: Replace with a separate transfer command
        Util::BeginOneTimeCommand(gTransferCommand);
        extent = imageCreateInfo.extent;
        const auto imageBarrier = Util::ImageBarrier(
            image.currentLayout,
            vk::ImageLayout::eTransferDstOptimal,
            image,
            vk::ImageAspectFlagBits::eColor,
            loadAllMipMaps ? ddsImage.numMips : 1);
        Util::PipelineBarrier(gTransferCommand, imageBarrier);
        const auto buffer = Util::UploadToImage(
            gContext,
            gTransferCommand,
            gTransferQueue.index,
            ddsImage.mipmaps,
            mipLevel,
            loadAllMipMaps,
            extent,
            image);
        Util::EndCommand(gTransferCommand);
        Util::SubmitQueueHost(gTransferQueue, gTransferCommand, gTransferFence);
        Util::WaitFence(gContext, gTransferFence);
        Util::ResetFence(gContext, gTransferFence);
        buffer.Destroy(gContext);
    }
    const auto arrayElement = static_cast<u32>(gSamplerImages.size() - 1);
    Util::UpdateDescriptorSampler(
        gDescriptor.set,
        gSamplerImages.back().imageView,
        gLinearSampler,
        arrayElement,
        gContext);
    return ImageObject().SetExtent({extent.width, extent.height}).SetIndex(arrayElement);
}

void Swift::DestroyImage(const ImageObject imageObject)
{
    auto& realImage = gWriteableImages.at(imageObject.index);
    const auto it = std::ranges::find_if(
        gWriteableImages,
        [&](const Image& image)
        {
            return image.image == realImage.image;
        });
    gWriteableImages.erase(it);
    realImage.Destroy(gContext);
}

BufferObject Swift::CreateBuffer(
    const BufferType bufferType,
    const u32 size)
{
    vk::BufferUsageFlags bufferUsageFlags = vk::BufferUsageFlagBits::eShaderDeviceAddress;
    switch (bufferType)
    {
    case BufferType::eUniform:
        bufferUsageFlags |= vk::BufferUsageFlagBits::eUniformBuffer;
        break;
    case BufferType::eStorage:
        bufferUsageFlags |= vk::BufferUsageFlagBits::eStorageBuffer;
        break;
    case BufferType::eIndex:
        bufferUsageFlags = vk::BufferUsageFlagBits::eIndexBuffer;
        break;
    }

    const auto [vulkanBuffer, allocation, info] =
        Init::CreateBuffer(gContext, gGraphicsQueue.index, size, bufferUsageFlags);
    const auto buffer =
        Buffer().SetBuffer(vulkanBuffer).SetAllocation(allocation).SetAllocationInfo(info);
    gBuffers.emplace_back(buffer);
    const auto index = static_cast<u32>(gBuffers.size() - 1);
    return BufferObject().SetIndex(index).SetSize(size);
}

void Swift::DestroyBuffer(const BufferObject bufferObject)
{
    const auto& realBuffer = gBuffers.at(bufferObject.index);
    const auto it = std::ranges::find_if(
        gBuffers,
        [&](const Buffer& buffer)
        {
            return buffer.buffer == realBuffer.buffer;
        });
    gBuffers.erase(it);
    realBuffer.Destroy(gContext);
}

void* Swift::MapBuffer(const BufferObject bufferObject)
{
    const auto& realBuffer = gBuffers.at(bufferObject.index);
    return Util::MapBuffer(gContext, realBuffer);
}

void Swift::UnmapBuffer(const BufferObject bufferObject)
{
    const auto& realBuffer = gBuffers.at(bufferObject.index);
    Util::UnmapBuffer(gContext, realBuffer);
}

void Swift::UploadToBuffer(
    const BufferObject& buffer,
    const void* data,
    const u64 offset,
    const u64 size)
{
    const auto& realBuffer = gBuffers.at(buffer.index);
    Util::UploadToBuffer(gContext, data, realBuffer, offset, size);
}

void Swift::UploadToMapped(
    void* mapped,
    const void* data,
    const u64 offset,
    const u64 size)
{
    Util::UploadToMapped(data, mapped, offset, size);
}

u64 Swift::GetBufferAddress(const BufferObject& buffer)
{
    const auto& realBuffer = gBuffers.at(buffer.index);
    const auto addressInfo = vk::BufferDeviceAddressInfo().setBuffer(realBuffer.buffer);
    return gContext.device.getBufferAddress(addressInfo);
}

void Swift::BindIndexBuffer(const BufferObject& bufferObject)
{
    const auto& realBuffer = gBuffers.at(bufferObject.index);
    const auto& commandBuffer = Render::GetCommandBuffer(gFrameData, gCurrentFrame);
    commandBuffer.bindIndexBuffer(realBuffer, 0, vk::IndexType::eUint32);
}

void Swift::CopyImage(
    const ImageObject srcImageObject,
    const ImageObject dstImageObject,
    const glm::uvec2 extent)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gFrameData, gCurrentFrame);
    constexpr auto srcLayout = vk::ImageLayout::eTransferSrcOptimal;
    constexpr auto dstLayout = vk::ImageLayout::eTransferDstOptimal;
    auto& srcImage = gWriteableImages.at(srcImageObject.index);
    auto& dstImage = gWriteableImages.at(dstImageObject.index);
    const auto srcBarrier = Util::ImageBarrier(
        srcImage.currentLayout,
        srcLayout,
        srcImage,
        vk::ImageAspectFlagBits::eColor);
    const auto dstBarrier = Util::ImageBarrier(
        dstImage.currentLayout,
        dstLayout,
        dstImage,
        vk::ImageAspectFlagBits::eColor);
    Util::PipelineBarrier(commandBuffer, {srcBarrier, dstBarrier});
    Util::CopyImage(commandBuffer, srcImage, srcLayout, dstImage, dstLayout, Util::To2D(extent));
}

void Swift::CopyToSwapchain(
    ImageObject srcImageObject,
    glm::uvec2 extent)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gFrameData, gCurrentFrame);
    constexpr auto srcLayout = vk::ImageLayout::eTransferSrcOptimal;
    constexpr auto dstLayout = vk::ImageLayout::eTransferDstOptimal;
    auto& srcImage = gWriteableImages.at(srcImageObject.index);
    auto& dstImage = gSwapchain.renderImage;
    const auto srcBarrier = Util::ImageBarrier(
        srcImage.currentLayout,
        srcLayout,
        srcImage,
        vk::ImageAspectFlagBits::eColor);
    const auto dstBarrier = Util::ImageBarrier(
        dstImage.currentLayout,
        dstLayout,
        dstImage,
        vk::ImageAspectFlagBits::eColor);
    Util::PipelineBarrier(commandBuffer, {srcBarrier, dstBarrier});
    Util::CopyImage(commandBuffer, srcImage, srcLayout, dstImage, dstLayout, Util::To2D(extent));
}

void Swift::BlitImage(
    ImageObject srcImageObject,
    ImageObject dstImageObject,
    glm::uvec2 srcOffset,
    glm::uvec2 dstOffset)
{
    const auto commandBuffer = Render::GetCommandBuffer(gFrameData, gCurrentFrame);
    constexpr auto srcLayout = vk::ImageLayout::eTransferSrcOptimal;
    constexpr auto dstLayout = vk::ImageLayout::eTransferDstOptimal;

    auto& srcImage = gWriteableImages.at(srcImageObject.index);
    auto& dstImage = gWriteableImages.at(dstImageObject.index);
    const auto srcBarrier = Util::ImageBarrier(
        srcImage.currentLayout,
        srcLayout,
        srcImage,
        vk::ImageAspectFlagBits::eColor);
    const auto dstBarrier = Util::ImageBarrier(
        dstImage.currentLayout,
        dstLayout,
        dstImage,
        vk::ImageAspectFlagBits::eColor);
    Util::PipelineBarrier(commandBuffer, {srcBarrier, dstBarrier});
    Util::BlitImage(
        commandBuffer,
        srcImage,
        srcLayout,
        dstImage,
        dstLayout,
        Util::To2D(srcOffset),
        Util::To2D(dstOffset));
}

void Swift::BlitToSwapchain(
    const ImageObject srcImageObject,
    const glm::uvec2 srcExtent)
{
    const auto commandBuffer = Render::GetCommandBuffer(gFrameData, gCurrentFrame);
    constexpr auto srcLayout = vk::ImageLayout::eTransferSrcOptimal;
    constexpr auto dstLayout = vk::ImageLayout::eTransferDstOptimal;

    auto& srcImage = gWriteableImages.at(srcImageObject.index);
    auto& dstImage = gSwapchain.renderImage;
    const auto srcBarrier = Util::ImageBarrier(
        srcImage.currentLayout,
        srcLayout,
        srcImage,
        vk::ImageAspectFlagBits::eColor);
    const auto dstBarrier = Util::ImageBarrier(
        dstImage.currentLayout,
        dstLayout,
        dstImage,
        vk::ImageAspectFlagBits::eColor);
    Util::PipelineBarrier(commandBuffer, {srcBarrier, dstBarrier});
    Util::BlitImage(
        commandBuffer,
        srcImage,
        srcLayout,
        dstImage,
        dstLayout,
        Util::To2D(srcExtent),
        gSwapchain.extent);
}

void Swift::DispatchCompute(
    const u32 x,
    const u32 y,
    const u32 z)
{
    const auto commandBuffer = Render::GetCommandBuffer(gFrameData, gCurrentFrame);
    commandBuffer.dispatch(x, y, z);
}

void Swift::PushConstant(
    const void* value,
    const u32 size)
{
    const auto commandBuffer = Render::GetCommandBuffer(gFrameData, gCurrentFrame);
    const auto [shaders, stageFlags, pipelineLayout] = gShaders.at(gCurrentShader);

    vk::ShaderStageFlags pushStageFlags;
    if (std::ranges::find(stageFlags, vk::ShaderStageFlagBits::eVertex) != stageFlags.end())
    {
        pushStageFlags = vk::ShaderStageFlagBits::eAllGraphics;
    }
    else if (std::ranges::find(stageFlags, vk::ShaderStageFlagBits::eCompute) != stageFlags.end())
    {
        pushStageFlags = vk::ShaderStageFlagBits::eCompute;
    }

    commandBuffer.pushConstants(pipelineLayout, pushStageFlags, 0, size, value);
}