#pragma once
#include "SwiftStructs.hpp"



namespace Swift::Renderer
{
    void Init(const InitInfo& initInfo);
    void Shutdown();

    void BeginFrame(const DynamicInfo& dynamicInfo);
    void EndFrame(const DynamicInfo& dynamicInfo);

    void BeginRendering();
    void EndRendering();

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

    ShaderObject CreateGraphicsShaderObject(
        std::string_view vertexPath,
        std::string_view fragmentPath,
        u32 pushConstantSize = 0);
    ShaderObject CreateComputeShaderObject(
        const std::string& computePath,
        u32 pushConstantSize = 0);
    void BindShader(const ShaderObject& shaderObject);

    ImageObject CreateWriteableImage(glm::uvec2 size);
    ImageObject CreateImageFromFile(
        const std::filesystem::path& filePath,
        int mipLevel,
        bool loadAllMipMaps);
    void DestroyImage(ImageObject imageObject);

    BufferObject CreateBuffer(
        BufferType bufferType,
        u32 size);
    void DestroyBuffer(BufferObject bufferObject);
    void UploadToBuffer(
        const BufferObject& buffer,
        const void* data,
        VkDeviceSize offset,
        VkDeviceSize size);
    u64 GetBufferAddress(const BufferObject& buffer);
    void BindIndexBuffer(const BufferObject& bufferObject);

    void CopyImage(
        ImageObject srcImageObject,
        ImageObject dstImageObject,
        glm::uvec2 extent);
    void CopyToSwapchain(
        ImageObject srcImageObject,
        glm::uvec2 extent);
    void BlitImage(
        ImageObject srcImageObject,
        ImageObject dstImageObject,
        glm::uvec2 srcOffset,
        glm::uvec2 dstOffset);
    void BlitToSwapchain(
        ImageObject srcImageObject,
        glm::uvec2 srcExtent);

    void DispatchCompute(
        u32 x,
        u32 y,
        u32 z);
    
    void PushConstant(void* value, u32 size);
} // namespace Swift::Renderer
