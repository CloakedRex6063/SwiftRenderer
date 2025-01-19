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
        const BufferHandle& buffer,
        u64 offset,
        u32 drawCount,
        u32 stride);

    ShaderHandle CreateGraphicsShaderHandle(
        std::string_view vertexPath,
        std::string_view fragmentPath,
        std::string_view debugName,
        u32 pushConstantSize = 0);

    ShaderHandle CreateComputeShaderHandle(
        const std::string& computePath,
        u32 pushConstantSize,
        std::string_view debugName);

    void BindShader(const ShaderHandle& shaderObject);

    void PushConstant(
        const void* value,
        u32 size);

    void DispatchCompute(
        u32 x,
        u32 y,
        u32 z);

    ImageHandle CreateWriteableImage(
        glm::uvec2 size,
        std::string_view debugName);
    ImageHandle LoadImageFromFile(
        const std::filesystem::path& filePath,
        int mipLevel,
        bool loadAllMipMaps,
        std::string_view debugName);
    void DestroyImage(ImageHandle imageHandle);
    ImageHandle LoadImageFromFileQueued(
        const std::filesystem::path& filePath,
        int mipLevel,
        bool loadAllMipMaps,
        std::string_view debugName);
    ImageHandle LoadCubemapFromFile(
        const std::filesystem::path& filePath,
        std::string_view debugName);
    std::tuple<
        ImageHandle,
        ImageHandle,
        ImageHandle,
        ImageHandle>
    LoadIBLDataFromHDRI(
        const std::filesystem::path& filePath,
        std::string_view debugName);

    u32 GetImageArrayIndex(ImageHandle imageHandle);

    BufferHandle CreateBuffer(
        BufferType bufferType,
        u32 size,
        std::string_view debugName);
    void DestroyBuffer(BufferHandle bufferHandle);

    void* MapBuffer(BufferHandle bufferHandle);
    void UnmapBuffer(BufferHandle bufferHandle);
    void UploadToBuffer(
        const BufferHandle& buffer,
        const void* data,
        u64 offset,
        u64 size);
    void UploadToMapped(
        void* mapped,
        const void* data,
        u64 offset,
        u64 size);
    void UpdateSmallBuffer(
        const BufferHandle& buffer,
        u64 offset,
        u64 size,
        const void* data);
    void CopyBuffer(
        BufferHandle srcBufferHandle,
        BufferHandle dstBufferHandle,
        u64 srcOffset,
        u64 dstOffset,
        u64 size);

    u64 GetBufferAddress(const BufferHandle& buffer);
    void BindIndexBuffer(const BufferHandle& bufferObject);

    void ClearImage(
        ImageHandle image,
        glm::vec4 color);
    void ClearSwapchainImage(glm::vec4 color);
    void CopyImage(
        ImageHandle srcImageHandle,
        ImageHandle dstImageHandle,
        glm::uvec2 extent);
    void CopyToSwapchain(
        ImageHandle srcImageHandle,
        glm::uvec2 extent);
    void BlitImage(
        ImageHandle srcImageHandle,
        ImageHandle dstImageHandle,
        glm::uvec2 srcOffset,
        glm::uvec2 dstOffset);
    void BlitToSwapchain(
        ImageHandle srcImageHandle,
        glm::uvec2 srcExtent);

    void BeginTransfer();
    void EndTransfer();
} // namespace Swift
