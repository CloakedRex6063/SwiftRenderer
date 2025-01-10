#include "Swift.hpp"
#include "Vulkan/VulkanInit.hpp"
#include "Vulkan/VulkanRender.hpp"
#include "Vulkan/VulkanStructs.hpp"
#include "Vulkan/VulkanUtil.hpp"
#include "dds.hpp"
#ifdef SWIFT_IMGUI
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#endif
#ifdef SWIFT_IMGUI_GLFW
#include "imgui_impl_glfw.h"
#endif

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <SwiftPCH.hpp>

namespace
{
    using namespace Swift;
    Vulkan::Context gContext;
    Vulkan::Queue gGraphicsQueue;
    Vulkan::Queue gComputeQueue;
    Vulkan::Queue gTransferQueue;
    Vulkan::Command gTransferCommand;
    Vulkan::Command gGraphicsCommand; // For non render loop operations
    vk::Fence gTransferFence;
    vk::Fence gGraphicsFence; // For non render loop operations

    Vulkan::Swapchain gSwapchain;
    Vulkan::BindlessDescriptor gDescriptor;
    // TODO: sampler pool for all types of samplers
    vk::Sampler gLinearSampler;

    std::vector<Vulkan::Buffer> gTransferStagingBuffers;
    std::vector<Vulkan::Buffer> gBuffers;
    std::vector<Vulkan::Shader> gShaders;
    u32 gCurrentShader = 0;
    std::vector<Vulkan::Image> gWriteableImages;
    std::vector<Vulkan::Image> gSamplerImages;
    std::vector<Vulkan::Image> gSamplerCubeImages;

    std::vector<Vulkan::FrameData> gFrameData;
    u32 gCurrentFrame = 0;
    Vulkan::FrameData gCurrentFrameData;

    InitInfo gInitInfo;
} // namespace

using namespace Vulkan;

void Swift::Init(const InitInfo& initInfo)
{
    gInitInfo = initInfo;

    gContext = Init::CreateContext(initInfo);

    const auto indices = Util::GetQueueFamilyIndices(gContext.gpu, gContext.surface);
    const auto graphicsQueue = Init::GetQueue(gContext, indices[0], "Graphics Queue");
    gGraphicsQueue.SetIndex(indices[0]).SetQueue(graphicsQueue);
    const auto computeQueue = Init::GetQueue(gContext, indices[1], "Compute Queue");
    gComputeQueue.SetIndex(indices[1]).SetQueue(computeQueue);
    const auto transferQueue = Init::GetQueue(gContext, indices[2], "Transfer Queue");
    gTransferQueue.SetIndex(indices[2]).SetQueue(transferQueue);

    constexpr auto depthFormat = vk::Format::eD32Sfloat;
    const auto depthImage = Init::CreateImage(
        gContext,
        vk::ImageType::e2D,
        Util::To3D(initInfo.extent),
        depthFormat,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        1,
        {},
        "Swapchain Depth");

    const auto swapchain =
        Init::CreateSwapchain(gContext, Util::To2D(initInfo.extent), gGraphicsQueue.index);
    gSwapchain.SetSwapchain(swapchain)
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
        .SetDescriptorPool(Init::CreateDescriptorPool(gContext, {}))
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

    gGraphicsCommand.commandPool =
        Init::CreateCommandPool(gContext, gGraphicsQueue.index, "Graphics Command Pool");
    gGraphicsCommand.commandBuffer = Init::CreateCommandBuffer(
        gContext,
        gGraphicsCommand.commandPool,
        "Graphics Command Buffer");
    gGraphicsFence = Init::CreateFence(gContext, {}, "Graphics Fence");

#ifdef SWIFT_IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    auto format = VK_FORMAT_B8G8R8A8_UNORM;
    ImGui_ImplVulkan_InitInfo vulkanInitInfo{
        .Instance = gContext.instance,
        .PhysicalDevice = gContext.gpu,
        .Device = gContext.device,
        .QueueFamily = gGraphicsQueue.index,
        .Queue = gGraphicsQueue.queue,
        .MinImageCount = Util::GetSwapchainImageCount(gContext.gpu, gContext.surface),
        .ImageCount = Util::GetSwapchainImageCount(gContext.gpu, gContext.surface),
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .DescriptorPoolSize = 1000,
        .UseDynamicRendering = true,
        .PipelineRenderingCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &format,
            .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
        }};
    ImGui_ImplVulkan_Init(&vulkanInitInfo);
    ImGui_ImplVulkan_CreateFontsTexture();
#endif
}

void Swift::Shutdown()
{
    const auto result = gContext.device.waitIdle();
    VK_ASSERT(result, "Failed to wait for device while cleaning up");

#ifdef SWIFT_IMGUI
    ImGui_ImplVulkan_Shutdown();
#ifdef SWIFT_IMGUI_GLFW
    ImGui_ImplGlfw_Shutdown();
#endif
    ImGui::DestroyContext();
#endif

    for (const auto& frameData : gFrameData)
    {
        frameData.Destroy(gContext);
    }
    gDescriptor.Destroy(gContext);

    for (const auto& shader : gShaders)
    {
        shader.Destroy(gContext);
    }

    for (auto& image : gWriteableImages)
    {
        image.Destroy(gContext);
    }

    for (auto& image : gSamplerImages)
    {
        image.Destroy(gContext);
    }

    for (auto& image : gSamplerCubeImages)
    {
        image.Destroy(gContext);
    }

    for (auto& buffer : gBuffers)
    {
        buffer.Destroy(gContext);
    }

    gTransferCommand.Destroy(gContext);
    gGraphicsCommand.Destroy(gContext);
    gContext.device.destroy(gTransferFence);
    gContext.device.destroy(gGraphicsFence);
    gContext.device.destroy(gLinearSampler);

    gSwapchain.Destroy(gContext);
    gContext.Destroy();
}

void Swift::BeginFrame(const DynamicInfo& dynamicInfo)
{
    gCurrentFrameData = gFrameData[gCurrentFrame];
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    const auto& renderFence = Render::GetRenderFence(gCurrentFrameData);

#ifdef SWIFT_IMGUI
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
#endif

    Util::WaitFence(gContext, renderFence, 1000000000);
    gSwapchain.imageIndex = Render::AcquireNextImage(
        gGraphicsQueue,
        gContext,
        gSwapchain,
        gCurrentFrameData.renderSemaphore,
        Util::To2D(dynamicInfo.extent));
    Util::ResetFence(gContext, renderFence);
    Util::BeginOneTimeCommand(commandBuffer);
}

void Swift::EndFrame(const DynamicInfo& dynamicInfo)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    const auto& renderSemaphore = Render::GetRenderSemaphore(gCurrentFrameData);
    const auto& presentSemaphore = Render::GetPresentSemaphore(gCurrentFrameData);
    const auto& renderFence = Render::GetRenderFence(gCurrentFrameData);
    auto& swapchainImage = Render::GetSwapchainImage(gSwapchain);

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
#ifdef SWIFT_IMGUI
    ImGui::EndFrame();
#endif
}

void Swift::BeginRendering()
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    Render::BeginRendering(commandBuffer, gSwapchain, true);
    Render::SetPipelineDefault(gContext, commandBuffer, gSwapchain.extent, gInitInfo.bUsePipelines);
    Render::EnableTransparencyBlending(gContext, commandBuffer);
}

void Swift::EndRendering()
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    commandBuffer.endRendering();

#ifdef SWIFT_IMGUI
    Render::BeginRendering(commandBuffer, gSwapchain, false);
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    commandBuffer.endRendering();
#endif
}

void Swift::ShowDebugStats()
{
#ifdef SWIFT_IMGUI
    VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
    vmaGetHeapBudgets(gContext.allocator, budgets);
    ImGui::Begin("Debug Statistics");
    ImGui::Text(
        "Memory Usage: %f GB",
        static_cast<float>(budgets[0].statistics.allocationBytes) / (1024.0f * 1024.0f * 1024.0f));
    ImGui::Text(
        "Memory Allocated: %f GB",
        static_cast<float>(budgets[0].usage) / (1024.0f * 1024.0f * 1024.0f));
    ImGui::Text(
        "Available GPU Memory: %f GB",
        static_cast<float>(budgets[0].budget) / (1024.0f * 1024.0f * 1024.0f));
    ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
    ImGui::End();
#endif
}

void Swift::SetCullMode(const CullMode& cullMode)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    Render::SetCullMode(commandBuffer, static_cast<vk::CullModeFlagBits>(cullMode));
}

void Swift::SetDepthCompareOp(DepthCompareOp depthCompareOp)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    Render::SetDepthCompareOp(commandBuffer, static_cast<vk::CompareOp>(depthCompareOp));
}
void Swift::SetPolygonMode(PolygonMode polygonMode)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    Render::SetPolygonMode(gContext, commandBuffer, static_cast<vk::PolygonMode>(polygonMode));
}

ShaderObject Swift::CreateGraphicsShaderObject(
    const std::string_view vertexPath,
    const std::string_view fragmentPath,
    const std::string_view debugName,
    const u32 pushConstantSize)
{
    const auto shader = Init::CreateGraphicsShader(
        gContext,
        gDescriptor,
        gInitInfo.bUsePipelines,
        pushConstantSize,
        vertexPath,
        fragmentPath,
        debugName);
    gShaders.emplace_back(shader);
    const auto index = static_cast<u32>(gShaders.size() - 1);
    return ShaderObject().SetIndex(index);
}

ShaderObject Swift::CreateComputeShaderObject(
    const std::string& computePath,
    const u32 pushConstantSize,
    const std::string_view debugName)
{
    const auto shader = Init::CreateComputeShader(
        gContext,
        gDescriptor,
        gInitInfo.bUsePipelines,
        pushConstantSize,
        computePath,
        debugName);
    gShaders.emplace_back(shader);
    const auto index = static_cast<u32>(gShaders.size() - 1);
    return ShaderObject().SetIndex(index);
}

void Swift::BindShader(const ShaderObject& shaderObject)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    const auto& shader = gShaders.at(shaderObject.index);

    Render::BindShader(commandBuffer, gContext, gDescriptor, shader, gInitInfo.bUsePipelines);

    gCurrentShader = shaderObject.index;
}

void Swift::Draw(
    const u32 vertexCount,
    const u32 instanceCount,
    const u32 firstVertex,
    const u32 firstInstance)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    commandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void Swift::DrawIndexed(
    const u32 indexCount,
    const u32 instanceCount,
    const u32 firstIndex,
    const int vertexOffset,
    const u32 firstInstance)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    commandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void Swift::DrawIndexedIndirect(
    const BufferObject& buffer,
    const u64 offset,
    const u32 drawCount,
    const u32 stride)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    const auto& realBuffer = gBuffers[buffer.index];
    commandBuffer.drawIndexedIndirect(realBuffer, offset, drawCount, stride);
}

ImageObject Swift::CreateWriteableImage(
    const glm::uvec2 size,
    const std::string_view debugName)
{
    constexpr auto imageUsage = vk::ImageUsageFlagBits::eColorAttachment |
                                vk::ImageUsageFlagBits::eTransferSrc |
                                vk::ImageUsageFlagBits::eTransferDst |
                                vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;

    constexpr auto format = vk::Format::eR16G16B16A16Sfloat;
    const auto image = Init::CreateImage(
        gContext,
        vk::ImageType::e2D,
        Util::To3D(size),
        format,
        imageUsage,
        1,
        {},
        debugName);
    gWriteableImages.emplace_back(image);

    const auto arrayElement = static_cast<u32>(gWriteableImages.size() - 1);
    Util::UpdateDescriptorImage(
        gDescriptor.set,
        image.imageView,
        gLinearSampler,
        arrayElement,
        gContext);

    return ImageObject().SetExtent(size).SetIndex(static_cast<u32>(gWriteableImages.size() - 1));
}

ImageObject Swift::LoadImageFromFile(
    const std::filesystem::path& filePath,
    const int mipLevel,
    const bool loadAllMipMaps,
    const std::string_view debugName)
{
    Swift::BeginTransfer();
    const auto image =
        Swift::LoadImageFromFileQueued(filePath, mipLevel, loadAllMipMaps, debugName);
    Swift::EndTransfer();
    return image;
}

ImageObject Swift::LoadImageFromFileQueued(
    const std::filesystem::path& filePath,
    const int mipLevel,
    const bool loadAllMipMaps,
    const std::string_view debugName)
{
    Image image;
    Buffer staging;
    if (filePath.extension() == ".dds")
    {
        std::tie(image, staging) = Init::CreateDDSImage(
            gContext,
            gTransferQueue,
            gTransferCommand,
            filePath,
            mipLevel,
            loadAllMipMaps,
            debugName);
    }
    gTransferStagingBuffers.emplace_back(staging);

    gSamplerImages.emplace_back(image);
    const auto arrayElement = static_cast<u32>(gSamplerImages.size() - 1);
    Util::UpdateDescriptorSampler(
        gDescriptor.set,
        gSamplerImages.back().imageView,
        gLinearSampler,
        arrayElement,
        gContext);
    return ImageObject()
        .SetExtent({image.extent.width, image.extent.height})
        .SetIndex(arrayElement);
}

ImageObject Swift::LoadCubemapFromFile(
    const std::filesystem::path& filePath,
    const std::string_view debugName)
{
    Swift::BeginTransfer();
    Image image;
    Buffer staging;
    assert(filePath.extension() == ".dds");
    if (filePath.extension() == ".dds")
    {
        std::tie(image, staging) = Init::CreateDDSImage(
            gContext,
            gTransferQueue,
            gTransferCommand,
            filePath,
            0,
            true,
            debugName);
    }
    Swift::EndTransfer();
    staging.Destroy(gContext);

    gSamplerCubeImages.emplace_back(image);
    const auto arrayElement = static_cast<u32>(gSamplerCubeImages.size() - 1);
    Util::UpdateDescriptorSamplerCube(
        gDescriptor.set,
        gSamplerCubeImages.back().imageView,
        gLinearSampler,
        arrayElement,
        gContext);

    return ImageObject()
        .SetExtent({image.extent.width, image.extent.height})
        .SetIndex(arrayElement);
}

std::tuple<
    ImageObject,
    ImageObject,
    ImageObject,
    ImageObject>
Swift::LoadIBLDataFromHDRI(
    const std::filesystem::path& filePath,
    const std::string_view debugName)
{
    const auto hdriObject = Swift::LoadImageFromFile(filePath, 0, false, debugName);

    const auto [skybox, irradiance, specular, lut] = Init::EquiRectangularToCubemap(
        gContext,
        gDescriptor,
        gSamplerCubeImages,
        gLinearSampler,
        gGraphicsCommand,
        gGraphicsFence,
        gGraphicsQueue,
        gInitInfo.bUsePipelines,
        hdriObject.index,
        debugName);

    auto arrayElement = static_cast<u32>(gSamplerCubeImages.size() - 1);
    Util::UpdateDescriptorSamplerCube(
        gDescriptor.set,
        gSamplerCubeImages.back().imageView,
        gLinearSampler,
        arrayElement,
        gContext);
    auto skyboxObject = ImageObject().SetIndex(arrayElement).SetType(ImageType::eReadOnly);

    gSamplerCubeImages.emplace_back(irradiance);
    arrayElement = static_cast<u32>(gSamplerCubeImages.size() - 1);
    Util::UpdateDescriptorSamplerCube(
        gDescriptor.set,
        gSamplerCubeImages.back().imageView,
        gLinearSampler,
        arrayElement,
        gContext);
    auto irradianceObject = ImageObject().SetIndex(arrayElement).SetType(ImageType::eReadOnly);

    gSamplerCubeImages.emplace_back(specular);
    arrayElement = static_cast<u32>(gSamplerCubeImages.size() - 1);
    Util::UpdateDescriptorSamplerCube(
        gDescriptor.set,
        gSamplerCubeImages.back().imageView,
        gLinearSampler,
        arrayElement,
        gContext);
    auto specularObject = ImageObject().SetIndex(arrayElement).SetType(ImageType::eReadOnly);
    
    gSamplerImages.emplace_back(lut);
    arrayElement = static_cast<u32>(gSamplerImages.size() - 1);
    Util::UpdateDescriptorSampler(
        gDescriptor.set,
        gSamplerImages.back().imageView,
        gLinearSampler,
        arrayElement,
        gContext);
    
    auto lutObject = ImageObject().SetIndex(arrayElement).SetType(ImageType::eReadOnly);

    return {skyboxObject, irradianceObject, specularObject, lutObject};
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
    const u32 size,
    const std::string_view debugName)
{
    vk::BufferUsageFlags bufferUsageFlags =
        vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst;
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
    case BufferType::eIndirect:
        bufferUsageFlags |= vk::BufferUsageFlagBits::eIndirectBuffer;
        break;
    }

    const auto buffer =
        Init::CreateBuffer(gContext, gGraphicsQueue.index, size, bufferUsageFlags, debugName);
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

void Swift::UpdateSmallBuffer(
    const BufferObject& buffer,
    const u64 offset,
    const u64 size,
    const void* data)
{
    const auto& realBuffer = gBuffers.at(buffer.index);
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    commandBuffer.updateBuffer(realBuffer, offset, size, data);
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
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    commandBuffer.bindIndexBuffer(realBuffer, 0, vk::IndexType::eUint32);
}

void Swift::ClearImage(
    const ImageObject image,
    const glm::vec4 color)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    auto& realImage = Util::GetRealImage(image, gSamplerImages, gWriteableImages);
    const auto generalBarrier = Util::ImageBarrier(
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eGeneral,
        realImage,
        vk::ImageAspectFlagBits::eColor);
    Util::PipelineBarrier(commandBuffer, generalBarrier);

    const auto clearColor = vk::ClearColorValue(color.x, color.y, color.z, color.w);
    Util::ClearColorImage(commandBuffer, realImage, clearColor);

    const auto colorBarrier = Util::ImageBarrier(
        vk::ImageLayout::eGeneral,
        vk::ImageLayout::eColorAttachmentOptimal,
        realImage,
        vk::ImageAspectFlagBits::eColor);
    Util::PipelineBarrier(commandBuffer, colorBarrier);
}

void Swift::ClearSwapchainImage(const glm::vec4 color)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    const auto generalBarrier = Util::ImageBarrier(
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eGeneral,
        Render::GetSwapchainImage(gSwapchain),
        vk::ImageAspectFlagBits::eColor);
    Util::PipelineBarrier(commandBuffer, generalBarrier);

    const auto clearColor = vk::ClearColorValue(color.x, color.y, color.z, color.w);
    Util::ClearColorImage(commandBuffer, Render::GetSwapchainImage(gSwapchain), clearColor);

    const auto colorBarrier = Util::ImageBarrier(
        vk::ImageLayout::eGeneral,
        vk::ImageLayout::eColorAttachmentOptimal,
        Render::GetSwapchainImage(gSwapchain),
        vk::ImageAspectFlagBits::eColor);
    Util::PipelineBarrier(commandBuffer, colorBarrier);
}

void Swift::CopyImage(
    const ImageObject srcImageObject,
    const ImageObject dstImageObject,
    const glm::uvec2 extent)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    constexpr auto srcLayout = vk::ImageLayout::eTransferSrcOptimal;
    constexpr auto dstLayout = vk::ImageLayout::eTransferDstOptimal;
    auto& srcImage = Util::GetRealImage(srcImageObject, gSamplerImages, gWriteableImages);
    auto& dstImage = Util::GetRealImage(dstImageObject, gSamplerImages, gWriteableImages);
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
    const ImageObject srcImageObject,
    const glm::uvec2 extent)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    constexpr auto srcLayout = vk::ImageLayout::eTransferSrcOptimal;
    constexpr auto dstLayout = vk::ImageLayout::eTransferDstOptimal;
    auto& srcImage = Util::GetRealImage(srcImageObject, gSamplerImages, gWriteableImages);
    auto& dstImage = Render::GetSwapchainImage(gSwapchain);
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
    const auto commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    constexpr auto srcLayout = vk::ImageLayout::eTransferSrcOptimal;
    constexpr auto dstLayout = vk::ImageLayout::eTransferDstOptimal;

    auto& srcImage = Util::GetRealImage(srcImageObject, gSamplerImages, gWriteableImages);
    ;
    auto& dstImage = Util::GetRealImage(dstImageObject, gSamplerImages, gWriteableImages);
    ;
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
    const auto commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    constexpr auto srcLayout = vk::ImageLayout::eTransferSrcOptimal;
    constexpr auto dstLayout = vk::ImageLayout::eTransferDstOptimal;

    auto& srcImage = Util::GetRealImage(srcImageObject, gSamplerImages, gWriteableImages);
    auto& dstImage = Render::GetSwapchainImage(gSwapchain);
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
    const auto commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    commandBuffer.dispatch(x, y, z);
}

void Swift::PushConstant(
    const void* value,
    const u32 size)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    const auto& [shaders, stageFlags, pipeline, pipelineLayout] = gShaders.at(gCurrentShader);

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

void Swift::BeginTransfer()
{
    Util::BeginOneTimeCommand(gTransferCommand);
}

void Swift::EndTransfer()
{
    Util::EndCommand(gTransferCommand);
    Util::SubmitQueueHost(gTransferQueue, gTransferCommand, gTransferFence);
    Util::WaitFence(gContext, gTransferFence);
    Util::ResetFence(gContext, gTransferFence);
    for (const auto& buffer : gTransferStagingBuffers)
    {
        buffer.Destroy(gContext);
    }
    gTransferStagingBuffers.clear();
}
