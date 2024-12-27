#include "Swift.hpp"
#include "Utils/FileIO.hpp"
#include "Vulkan/VulkanInit.hpp"
#include "Vulkan/VulkanRender.hpp"
#include "Vulkan/VulkanUtil.hpp"
#include "dds.hpp"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

using namespace Swift::Vulkan;

namespace Swift
{
    void Renderer::Init(const InitInfo& initInfo)
    {
        mContext.SetInstance(Init::CreateInstance(initInfo.appName, initInfo.engineName))
            .SetSurface(Init::CreateSurface(mContext.instance, initInfo.hwnd))
            .SetGPU(Init::ChooseGPU(mContext.instance, mContext.surface))
            .SetDevice(Init::CreateDevice(mContext.gpu, mContext.surface))
            .SetAllocator(Init::CreateAllocator(mContext))
            .SetDynamicLoader(Init::CreateDynamicLoader(mContext.instance, mContext.device));

        const auto indices = Util::GetQueueFamilyIndices(mContext.gpu, mContext.surface);
        const auto graphicsQueue = Init::GetQueue(mContext, indices[0], "Graphics Queue");
        mGraphicsQueue.SetIndex(indices[0]).SetQueue(graphicsQueue);
        const auto computeQueue = Init::GetQueue(mContext, indices[1], "Compute Queue");
        mComputeQueue.SetIndex(indices[1]).SetQueue(computeQueue);
        const auto transferQueue = Init::GetQueue(mContext, indices[2], "Transfer Queue");
        mTransferQueue.SetIndex(indices[2]).SetQueue(transferQueue);

        constexpr auto renderFormat = vk::Format::eR16G16B16A16Sfloat;
        auto [renderVkImage, renderAlloc] = Init::CreateImage(
            mContext.allocator,
            Util::To3D(initInfo.extent),
            vk::ImageType::e2D,
            renderFormat,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc |
                vk::ImageUsageFlagBits::eTransferDst);
        const auto renderView =
            Init::CreateColorImageView(mContext, renderVkImage, renderFormat, "Render Image");
        const auto renderImage = Image()
                                     .SetFormat(renderFormat)
                                     .SetImage(renderVkImage)
                                     .SetAllocation(renderAlloc)
                                     .SetView(renderView);

        constexpr auto depthFormat = vk::Format::eD32Sfloat;
        auto [depthVkImage, depthAlloc] = Init::CreateImage(
            mContext.allocator,
            Util::To3D(initInfo.extent),
            vk::ImageType::e2D,
            depthFormat,
            vk::ImageUsageFlagBits::eDepthStencilAttachment);
        const auto depthView = Init::CreateDepthImageView(mContext, depthVkImage, "Depth Image");
        const auto depthImage = Image()
                                    .SetFormat(depthFormat)
                                    .SetImage(depthVkImage)
                                    .SetAllocation(depthAlloc)
                                    .SetView(depthView);

        const auto swapchain =
            Init::CreateSwapchain(mContext, Util::To2D(initInfo.extent), mGraphicsQueue.index);
        mSwapchain.SetSwapchain(swapchain)
            .SetRenderImage(renderImage)
            .SetDepthImage(depthImage)
            .SetImages(Init::CreateSwapchainImages(mContext, mSwapchain))
            .SetExtent(Util::To2D(initInfo.extent));

        mFrameData.resize(mSwapchain.images.size());
        for (auto& [renderSemaphore, presentSemaphore, renderFence, renderCommand] : mFrameData)
        {
            renderFence =
                Init::CreateFence(mContext, vk::FenceCreateFlagBits::eSignaled, "Render Fence");
            renderSemaphore = Init::CreateSemaphore(mContext, "Render Semaphore");
            presentSemaphore = Init::CreateSemaphore(mContext, "Present Semaphore");
            renderCommand.commandPool =
                Init::CreateCommandPool(mContext, mGraphicsQueue.index, "Command Pool");
            renderCommand.commandBuffer =
                Init::CreateCommandBuffer(mContext, renderCommand.commandPool, "Command Buffer");
        }

        mDescriptor.SetDescriptorSetLayout(Init::CreateDescriptorSetLayout(mContext))
            .SetDescriptorPool(Init::CreateDescriptorPool(mContext))
            .SetDescriptorSet(
                Init::CreateDescriptorSet(mContext, mDescriptor.pool, mDescriptor.setLayout));

        constexpr auto samplerCreateInfo = vk::SamplerCreateInfo()
                                               .setMagFilter(vk::Filter::eLinear)
                                               .setMinFilter(vk::Filter::eLinear)
                                               .setAddressModeU(vk::SamplerAddressMode::eRepeat)
                                               .setAddressModeV(vk::SamplerAddressMode::eRepeat)
                                               .setAddressModeW(vk::SamplerAddressMode::eRepeat)
                                               .setBorderColor(vk::BorderColor::eIntOpaqueWhite)
                                               .setUnnormalizedCoordinates(false)
                                               .setCompareOp(vk::CompareOp::eAlways)
                                               .setCompareEnable(false)
                                               .setMipmapMode(vk::SamplerMipmapMode::eLinear).setMinLod(6).setMaxLod(50);
        vk::Result result;
        std::tie(result, mLinearSampler) = mContext.device.createSampler(samplerCreateInfo);
        VK_ASSERT(result, "Failed to create sampler!");

        mTransferCommand.commandPool =
            Init::CreateCommandPool(mContext, mTransferQueue.index, "Transfer Command Pool");
        mTransferCommand.commandBuffer = Init::CreateCommandBuffer(
            mContext,
            mTransferCommand.commandPool,
            "Transfer Command Buffer");
        mTransferFence = Init::CreateFence(mContext, {}, "Transfer Fence");
    }

    void Renderer::BeginFrame(const DynamicInfo& dynamicInfo)
    {
        const auto& commandBuffer = Render::GetCommandBuffer(mFrameData, mCurrentFrame);
        const auto& renderSemaphore = Render::GetRenderSemaphore(mFrameData, mCurrentFrame);
        const auto& renderFence = Render::GetRenderFence(mFrameData, mCurrentFrame);

        Util::WaitFence(mContext, renderFence, 1000000000);

        mSwapchain.imageIndex = Render::AcquireNextImage(
            mGraphicsQueue,
            mContext,
            mSwapchain,
            renderSemaphore,
            Util::To2D(dynamicInfo.extent));
        Util::ResetFence(mContext, renderFence);
        Util::BeginOneTimeCommand(commandBuffer);

        const auto generalBarrier = Util::ImageBarrier(
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eGeneral,
            Render::GetSwapchainImage(mSwapchain),
            vk::ImageAspectFlagBits::eColor);
        Util::PipelineBarrier(commandBuffer, generalBarrier);

        const auto& swapchainImage = Render::GetSwapchainImage(mSwapchain);
        Util::ClearColorImage(commandBuffer, swapchainImage);

        const auto colorBarrier = Util::ImageBarrier(
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eColorAttachmentOptimal,
            Render::GetSwapchainImage(mSwapchain),
            vk::ImageAspectFlagBits::eColor);
        Util::PipelineBarrier(commandBuffer, colorBarrier);
    }

    void Renderer::BeginRendering()
    {
        const auto& commandBuffer = Render::GetCommandBuffer(mFrameData, mCurrentFrame);
        Render::BeginRendering(commandBuffer, mSwapchain);
        Render::DefaultRenderConfig(commandBuffer, mSwapchain.extent, mContext.dynamicLoader);
    }

    void Renderer::Draw(
        const u32 vertexCount,
        const u32 instanceCount,
        const u32 firstVertex,
        const u32 firstInstance)
    {
        const auto& commandBuffer = Render::GetCommandBuffer(mFrameData, mCurrentFrame);
        commandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
    }
    void Renderer::DrawIndexed(
        const u32 indexCount,
        const u32 instanceCount,
        const u32 firstIndex,
        const int vertexOffset,
        const u32 firstInstance)
    {
        const auto& commandBuffer = Render::GetCommandBuffer(mFrameData, mCurrentFrame);
        commandBuffer
            .drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void Renderer::EndRendering()
    {
        const auto& commandBuffer = Render::GetCommandBuffer(mFrameData, mCurrentFrame);
        commandBuffer.endRendering();
    }

    void Renderer::EndFrame(const DynamicInfo& dynamicInfo)
    {
        const auto& commandBuffer = Render::GetCommandBuffer(mFrameData, mCurrentFrame);
        const auto& renderSemaphore = Render::GetRenderSemaphore(mFrameData, mCurrentFrame);
        const auto& presentSemaphore = Render::GetPresentSemaphore(mFrameData, mCurrentFrame);
        const auto& renderFence = Render::GetRenderFence(mFrameData, mCurrentFrame);
        auto& swapchainImage = Render::GetSwapchainImage(mSwapchain);

        const auto copyRenderBarrier = Util::ImageBarrier(
            mSwapchain.renderImage.currentLayout,
            vk::ImageLayout::eTransferSrcOptimal,
            mSwapchain.renderImage,
            vk::ImageAspectFlagBits::eColor);

        const auto copySwapchainBarrier = Util::ImageBarrier(
            swapchainImage.currentLayout,
            vk::ImageLayout::eTransferDstOptimal,
            swapchainImage,
            vk::ImageAspectFlagBits::eColor);

        Util::PipelineBarrier(commandBuffer, {copyRenderBarrier, copySwapchainBarrier});

        Util::BlitImage(
            commandBuffer,
            mSwapchain.renderImage,
            mSwapchain.renderImage.currentLayout,
            swapchainImage,
            swapchainImage.currentLayout,
            mSwapchain.extent,
            mSwapchain.extent);

        const auto presentBarrier = Util::ImageBarrier(
            swapchainImage.currentLayout,
            vk::ImageLayout::ePresentSrcKHR,
            swapchainImage,
            vk::ImageAspectFlagBits::eColor);
        Util::PipelineBarrier(commandBuffer, presentBarrier);

        Util::EndCommand(commandBuffer);
        Util::SubmitQueue(
            mGraphicsQueue,
            commandBuffer,
            renderSemaphore,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            presentSemaphore,
            vk::PipelineStageFlagBits2::eAllGraphics,
            renderFence);
        Render::Present(
            mContext,
            mSwapchain,
            mGraphicsQueue,
            presentSemaphore,
            Util::To2D(dynamicInfo.extent));

        mCurrentFrame = (mCurrentFrame + 1) % mSwapchain.images.size();
    }

    void Renderer::BindShader(const ShaderObject& shader)
    {
        const auto& commandBuffer = Render::GetCommandBuffer(mFrameData, mCurrentFrame);
        const auto& [shaders, stageFlags, pipelineLayout] = mShaders.at(shader.index);
        commandBuffer.bindShadersEXT(stageFlags, shaders, mContext.dynamicLoader);
        const auto it = std::ranges::find(stageFlags, vk::ShaderStageFlagBits::eCompute);
        auto pipelineBindPoint = vk::PipelineBindPoint::eCompute;
        if (it == std::end(stageFlags))
        {
            pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        }
        commandBuffer.bindDescriptorSets(pipelineBindPoint, pipelineLayout, 0, mDescriptor.set, {});

        mCurrentShader = shader.index;
    }

    ShaderObject Renderer::CreateGraphicsShaderObject(
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
                mDescriptor.setLayout,
                pushConstantRange,
                vk::ShaderCreateFlagBitsEXT::eLinkStage,
                vk::ShaderStageFlagBits::eFragment),
            Init::CreateShader(
                fragmentCode,
                vk::ShaderStageFlagBits::eFragment,
                mDescriptor.setLayout,
                pushConstantRange,
                vk::ShaderCreateFlagBitsEXT::eLinkStage),
        };

        auto [result, shaders] =
            mContext.device.createShadersEXT(shaderCreateInfo, nullptr, mContext.dynamicLoader);
        VK_ASSERT(result, "Failed to create shaders!");

        auto layoutCreateInfo = vk::PipelineLayoutCreateInfo().setSetLayouts(mDescriptor.setLayout);

        if (pushConstantSize > 0)
        {
            layoutCreateInfo.setPushConstantRanges(pushConstantRange);
        }

        const auto [pipeResult, pipelineLayout] =
            mContext.device.createPipelineLayout(layoutCreateInfo);
        VK_ASSERT(pipeResult, "Failed to create pipeline layout for shader!");

        constexpr auto stageFlags = {
            vk::ShaderStageFlagBits::eVertex,
            vk::ShaderStageFlagBits::eFragment};
        mShaders.emplace_back(
            Shader()
                .SetShaders(shaders)
                .SetPipelineLayout(pipelineLayout)
                .SetStageFlags(stageFlags));

        const auto index = static_cast<u32>(mShaders.size() - 1);
        const auto name = "Graphics Layout " + std::to_string(index);
        Util::NameObject(pipelineLayout, name, mContext);
        return ShaderObject().SetIndex(index);
    }

    ShaderObject Renderer::CreateComputeShaderObject(
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
            mDescriptor.setLayout,
            pushConstantRange);
        auto [result, shader] =
            mContext.device.createShaderEXT(shaderCreateInfo, nullptr, mContext.dynamicLoader);
        VK_ASSERT(result, "Failed to create shaders!");

        auto layoutCreateInfo = vk::PipelineLayoutCreateInfo().setSetLayouts(mDescriptor.setLayout);

        if (pushConstantSize > 0)
        {
            layoutCreateInfo.setPushConstantRanges(pushConstantRange);
        }

        const auto [pipeResult, pipelineLayout] =
            mContext.device.createPipelineLayout(layoutCreateInfo);
        VK_ASSERT(pipeResult, "Failed to create pipeline layout for shader!");

        mShaders.emplace_back(
            Shader()
                .SetShaders({shader})
                .SetPipelineLayout(pipelineLayout)
                .SetStageFlags({vk::ShaderStageFlagBits::eCompute}));
        const auto index = static_cast<u32>(mShaders.size() - 1);
        const auto name = "Compute Layout " + std::to_string(index);
        Util::NameObject(pipelineLayout, name, mContext);
        return ShaderObject().SetIndex(index);
    }

    ImageObject Renderer::CreateWriteableImage(const glm::uvec2 size)
    {
        constexpr auto imageUsage =
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc |
            vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eStorage |
            vk::ImageUsageFlagBits::eSampled;

        constexpr auto format = vk::Format::eR16G16B16A16Sfloat;
        auto [vkImage, allocation] = Init::CreateImage(
            mContext.allocator,
            Util::To3D(size),
            vk::ImageType::e2D,
            format,
            imageUsage);

        const auto view = Init::CreateColorImageView(mContext, vkImage, format, "Render Image");
        const auto renderImage =
            Image().SetFormat(format).SetImage(vkImage).SetAllocation(allocation).SetView(view);

        mWriteableImages.emplace_back(renderImage);

        const auto arrayElement = static_cast<u32>(mWriteableImages.size() - 1);
        Util::UpdateDescriptorImage(
            mDescriptor.set,
            renderImage.imageView,
            mLinearSampler,
            arrayElement,
            mContext);

        return ImageObject().SetExtent(size).SetIndex(
            static_cast<u32>(mWriteableImages.size() - 1));
    }

    ImageObject Renderer::CreateImageFromFile(
        const std::filesystem::path& filePath,
        const int mipLevel,
        const bool loadAllMipMaps)
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

            auto [vulkanImage, alloc] = Init::CreateImage(mContext, imageCreateInfo);
            Util::NameObject(vulkanImage, "Image", mContext);
            imageViewCreateInfo.image = vulkanImage;
            if (!loadAllMipMaps)
            {
                imageViewCreateInfo.subresourceRange.levelCount = 1;
            }
            const auto imageView = Init::CreateImageView(mContext, imageViewCreateInfo);

            auto image = Image()
                             .SetImage(vulkanImage)
                             .SetAllocation(alloc)
                             .SetView(imageView)
                             .SetFormat(static_cast<vk::Format>(imageViewCreateInfo.format));
            mSamplerImages.emplace_back(image);

            // TODO: Replace with a separate transfer command
            Util::BeginOneTimeCommand(mTransferCommand);
            extent = imageCreateInfo.extent;
            const auto imageBarrier = Util::ImageBarrier(
                image.currentLayout,
                vk::ImageLayout::eTransferDstOptimal,
                image,
                vk::ImageAspectFlagBits::eColor,
                loadAllMipMaps ? ddsImage.numMips : 1);
            Util::PipelineBarrier(mTransferCommand, imageBarrier);
            const auto buffer = Util::UploadToImage(
                mContext,
                mTransferCommand,
                mTransferQueue.index,
                ddsImage.mipmaps,
                mipLevel,
                loadAllMipMaps,
                extent,
                image);
            Util::EndCommand(mTransferCommand);
            Util::SubmitQueueHost(mTransferQueue, mTransferCommand, mTransferFence);
            Util::WaitFence(mContext, mTransferFence);
            buffer.Destroy(mContext);
        }
        const auto arrayElement = static_cast<u32>(mSamplerImages.size() - 1);
        Util::UpdateDescriptorSampler(
            mDescriptor.set,
            mSamplerImages.back().imageView,
            mLinearSampler,
            arrayElement,
            mContext);
        return ImageObject().SetExtent({extent.width, extent.height}).SetIndex(arrayElement);
    }

    void Renderer::DestroyImage(const ImageObject imageObject)
    {
        auto& realImage = mWriteableImages.at(imageObject.index);
        const auto it = std::ranges::find_if(
            mWriteableImages,
            [&](const Image& image)
            {
                return image.image == realImage.image;
            });
        mWriteableImages.erase(it);
        realImage.Destroy(mContext);
    }

    BufferObject Renderer::CreateBuffer(
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
            Init::CreateBuffer(mContext, mGraphicsQueue.index, size, bufferUsageFlags);
        const auto buffer =
            Buffer().SetBuffer(vulkanBuffer).SetAllocation(allocation).SetAllocationInfo(info);
        mBuffers.emplace_back(buffer);
        const auto index = static_cast<u32>(mBuffers.size() - 1);
        return BufferObject().SetIndex(index).SetSize(size);
    }

    void Renderer::DestroyBuffer(const BufferObject bufferObject)
    {
        const auto& realBuffer = mBuffers.at(bufferObject.index);
        const auto it = std::ranges::find_if(
            mBuffers,
            [&](const Buffer& buffer)
            {
                return buffer.buffer == realBuffer.buffer;
            });
        mBuffers.erase(it);
        realBuffer.Destroy(mContext);
    }

    void Renderer::UploadToBuffer(
        const BufferObject& buffer,
        const void* data,
        const VkDeviceSize offset,
        const VkDeviceSize size) const
    {
        const auto& realBuffer = mBuffers.at(buffer.index);
        Util::UploadToBuffer(mContext, data, realBuffer, offset, size);
    }

    u64 Renderer::GetBufferAddress(const BufferObject& buffer) const
    {
        auto& realBuffer = mBuffers.at(buffer.index);
        const auto addressInfo = vk::BufferDeviceAddressInfo().setBuffer(realBuffer.buffer);
        return mContext.device.getBufferAddress(addressInfo);
    }

    void Renderer::BindIndexBuffer(const BufferObject& bufferObject)
    {
        const auto& realBuffer = mBuffers.at(bufferObject.index);
        const auto& commandBuffer = Render::GetCommandBuffer(mFrameData, mCurrentFrame);
        commandBuffer.bindIndexBuffer(realBuffer, 0, vk::IndexType::eUint32);
    }

    void Renderer::CopyImage(
        const ImageObject srcImageObject,
        const ImageObject dstImageObject,
        const glm::uvec2 extent)
    {
        const auto& commandBuffer = Render::GetCommandBuffer(mFrameData, mCurrentFrame);
        constexpr auto srcLayout = vk::ImageLayout::eTransferSrcOptimal;
        constexpr auto dstLayout = vk::ImageLayout::eTransferDstOptimal;
        auto& srcImage = mWriteableImages.at(srcImageObject.index);
        auto& dstImage = mWriteableImages.at(dstImageObject.index);
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
        Util::CopyImage(
            commandBuffer,
            srcImage,
            srcLayout,
            dstImage,
            dstLayout,
            Util::To2D(extent));
    }

    void Renderer::CopyToSwapchain(
        const ImageObject srcImageObject,
        const glm::uvec2 extent)
    {
        const auto& commandBuffer = Render::GetCommandBuffer(mFrameData, mCurrentFrame);
        constexpr auto srcLayout = vk::ImageLayout::eTransferSrcOptimal;
        constexpr auto dstLayout = vk::ImageLayout::eTransferDstOptimal;
        auto& srcImage = mWriteableImages.at(srcImageObject.index);
        auto& dstImage = mSwapchain.renderImage;
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
        Util::CopyImage(
            commandBuffer,
            srcImage,
            srcLayout,
            dstImage,
            dstLayout,
            Util::To2D(extent));
    }

    void Renderer::BlitImage(
        const ImageObject srcImageObject,
        const ImageObject dstImageObject,
        const glm::uvec2 srcOffset,
        const glm::uvec2 dstOffset)
    {
        const auto commandBuffer = Render::GetCommandBuffer(mFrameData, mCurrentFrame);
        constexpr auto srcLayout = vk::ImageLayout::eTransferSrcOptimal;
        constexpr auto dstLayout = vk::ImageLayout::eTransferDstOptimal;

        auto& srcImage = mWriteableImages.at(srcImageObject.index);
        auto& dstImage = mWriteableImages.at(dstImageObject.index);
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

    void Renderer::BlitToSwapchain(
        const ImageObject srcImageObject,
        const glm::uvec2 srcExtent)
    {
        const auto commandBuffer = Render::GetCommandBuffer(mFrameData, mCurrentFrame);
        constexpr auto srcLayout = vk::ImageLayout::eTransferSrcOptimal;
        constexpr auto dstLayout = vk::ImageLayout::eTransferDstOptimal;

        auto& srcImage = mWriteableImages.at(srcImageObject.index);
        auto& dstImage = mSwapchain.renderImage;
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
            mSwapchain.extent);
    }

    void Renderer::DispatchCompute(
        const u32 x,
        const u32 y,
        const u32 z)
    {
        const auto commandBuffer = Render::GetCommandBuffer(mFrameData, mCurrentFrame);
        auto [shaders, stageFlags, layout] = mShaders.at(mCurrentShader);
        commandBuffer.dispatch(x, y, z);
    }

    void Renderer::Shutdown()
    {
        const auto result = mContext.device.waitIdle();
        VK_ASSERT(result, "Failed to wait for device while cleaning up");
        for (const auto& frameData : mFrameData)
        {
            frameData.Destroy(mContext);
        }

        mDescriptor.Destroy(mContext);

        for (const auto& [shaders, stageFlags, pipelineLayout] : mShaders)
        {
            for (const auto& shader : shaders)
            {
                mContext.device.destroy(shader, nullptr, mContext.dynamicLoader);
            }
            mContext.device.destroy(pipelineLayout);
        }

        for (auto& image : mWriteableImages)
        {
            image.Destroy(mContext);
        }

        for (auto& image : mSamplerImages)
        {
            image.Destroy(mContext);
        }

        for (auto& buffer : mBuffers)
        {
            buffer.Destroy(mContext);
        }

        mTransferCommand.Destroy(mContext);
        mContext.device.destroy(mTransferFence);

        mContext.device.destroy(mLinearSampler);

        mSwapchain.Destroy(mContext);
        mContext.Destroy();
    }
} // namespace Swift