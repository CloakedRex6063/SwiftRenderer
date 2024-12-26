#pragma once
#include "SwiftStructs.hpp"
#include "Vulkan/VulkanRender.hpp"
#include "Vulkan/VulkanStructs.hpp"

namespace Swift
{
    class Renderer
    {
    public:
        void Init(const InitInfo& initInfo);
        void Shutdown();

        void BeginFrame(const DynamicInfo& dynamicInfo);
        void BeginRendering();
        void Draw(
            u32 vertexCount,
            u32 instanceCount,
            u32 firstVertex,
            u32 firstInstance);
        void DrawIndexed(
            u32 indexCount,
            u32 instanceCount,
            u32 firstIndex,
            int vertexOffset,
            u32 firstInstance);
        void EndRendering();
        void BindShader(const ShaderObject& shader);
        void EndFrame(const DynamicInfo& dynamicInfo);

        ShaderObject CreateGraphicsShaderObject(
            std::string_view vertexPath,
            std::string_view fragmentPath,
            u32 pushConstantSize = 0);
        ShaderObject CreateComputeShaderObject(
            const std::string& computePath,
            u32 pushConstantSize = 0);

        ImageObject CreateWriteableImage(glm::uvec2 size);
        ImageObject CreateImageFromFile(const std::filesystem::path& filePath);
        void DestroyImage(ImageObject imageObject);

        BufferObject CreateBuffer(
            BufferType bufferType,
            u32 size);
        void DestroyBuffer(BufferObject bufferObject);

        void UploadToBuffer(
            const BufferObject& buffer,
            const void* data,
            VkDeviceSize offset,
            VkDeviceSize size) const;
        u64 GetBufferAddress(const BufferObject& buffer) const;
        void BindIndexBuffer(const BufferObject& bufferObject);

        void CopyImage(
            ImageObject srcImageObject,
            ImageObject dstImageObject,
            glm::uvec2 extent);

        void BlitImage(
            ImageObject srcImageObject,
            ImageObject dstImageObject,
            glm::uvec2 srcOffset,
            glm::uvec2 dstOffset);

        void CopyToSwapchain(
            ImageObject srcImageObject,
            glm::uvec2 extent);
        void BlitToSwapchain(
            ImageObject srcImageObject,
            glm::uvec2 srcExtent);

        void DispatchCompute(
            u32 x,
            u32 y,
            u32 z);

        template <class T> void PushConstant(T& value);

    private:
        Vulkan::Context mContext;
        Vulkan::Queue mGraphicsQueue;
        Vulkan::Queue mComputeQueue;
        Vulkan::Queue mTransferQueue;
        Vulkan::Command mTransferCommand;
        vk::Fence mTransferFence;
        
        Vulkan::Swapchain mSwapchain;
        Vulkan::BindlessDescriptor mDescriptor;
        // TODO: sampler pool for all types of samplers
        vk::Sampler mLinearSampler;

        std::vector<Vulkan::Buffer> mBuffers;
        std::vector<Vulkan::Shader> mShaders;
        u32 mCurrentShader = 0;
        std::vector<Vulkan::Image> mWriteableImages;
        std::vector<Vulkan::Image> mSamplerImages;
        std::vector<Vulkan::FrameData> mFrameData;
        u32 mCurrentFrame = 0;
        bool mResized = false;
    };
} // namespace Swift

template <typename T> void Swift::Renderer::PushConstant(T& value)
{
    const auto commandBuffer = Vulkan::Render::GetCommandBuffer(mFrameData, mCurrentFrame);
    const auto [shaders, stageFlags, pipelineLayout] = mShaders.at(mCurrentShader);

    vk::ShaderStageFlags pushStageFlags;
    if (std::ranges::find(stageFlags, vk::ShaderStageFlagBits::eVertex) != stageFlags.end())
    {
        pushStageFlags = vk::ShaderStageFlagBits::eAllGraphics;
    }
    else if (std::ranges::find(stageFlags, vk::ShaderStageFlagBits::eCompute) != stageFlags.end())
    {
        pushStageFlags = vk::ShaderStageFlagBits::eCompute;
    }

    commandBuffer.pushConstants(pipelineLayout, pushStageFlags, 0, sizeof(T), &value);
}