#pragma once
#include "SwiftStructs.hpp"

namespace Swift
{
    void Init(const InitInfo& initInfo);
    void Shutdown();

    void BeginFrame(const DynamicInfo& dynamicInfo);
    void EndFrame(const DynamicInfo& dynamicInfo);

    void BeginRendering();
    void EndRendering();

    void ShowDebugStats();

    void SetCullMode(const CullMode& cullMode);
    void SetDepthCompareOp(DepthCompareOp depthCompareOp);
    void SetPolygonMode(PolygonMode polygonMode);

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
    void DrawIndexedIndirect(
        const BufferObject& buffer,
        u64 offset,
        u32 drawCount,
        u32 stride);

    ShaderObject CreateGraphicsShaderObject(
        std::string_view vertexPath,
        std::string_view fragmentPath,
        std::string_view debugName,
        u32 pushConstantSize = 0);
    ShaderObject CreateComputeShaderObject(
        const std::string& computePath,
        u32 pushConstantSize,
        std::string_view debugName);
    void BindShader(const ShaderObject& shaderObject);

    ImageObject CreateWriteableImage(
        glm::uvec2 size,
        std::string_view debugName);
    ImageObject LoadImageFromFile(
        const std::filesystem::path& filePath,
        int mipLevel,
        bool loadAllMipMaps,
        std::string_view debugName);
    ImageObject LoadImageFromFileQueued(
        const std::filesystem::path& filePath,
        int mipLevel,
        bool loadAllMipMaps,
        std::string_view debugName);
    ImageObject LoadCubemapFromFile(
        const std::filesystem::path& filePath,
        std::string_view debugName);
    void DestroyImage(ImageObject imageObject);

    BufferObject CreateBuffer(
        BufferType bufferType,
        u32 size,
        std::string_view debugName);
    void DestroyBuffer(BufferObject bufferObject);

    void* MapBuffer(BufferObject bufferObject);
    void UnmapBuffer(BufferObject bufferObject);
    void UploadToBuffer(
        const BufferObject& buffer,
        const void* data,
        u64 offset,
        u64 size);
    void UploadToMapped(
        void* mapped,
        const void* data,
        u64 offset,
        u64 size);
    void UpdateSmallBuffer(
        const BufferObject& buffer,
        u64 offset,
        u64 size,
        const void* data);

    u64 GetBufferAddress(const BufferObject& buffer);
    void BindIndexBuffer(const BufferObject& bufferObject);

    void ClearImage(
        ImageObject image,
        glm::vec4 color);
    void ClearSwapchainImage(glm::vec4 color);
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
    void PushConstant(
        const void* value,
        u32 size);

    void BeginTransfer();
    void EndTransfer();
} // namespace Swift
