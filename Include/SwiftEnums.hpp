#pragma once

namespace Swift
{
    enum class ImageFormat : uint8_t
    {
        eR16G16B16A16_SRGB,
        eR32G32B32A32_SRGB,
    };

    enum class ImageType : uint8_t
    {
        eReadWrite,
        eReadOnly,
        eTemporary,
    };

    enum class BufferType : uint8_t
    {
        eUniform,
        eStorage,
        eIndex,
        eIndirect,
        eReadback
    };

    enum class CullMode
    {
        eNone,
        eFront,
        eBack,
        eFrontAndBack
    };

    enum class DepthCompareOp
    {
        eNever,
        eLess,
        eEqual,
        eLessOrEqual,
        eGreater,
        eNotEqual,
        eGreaterOrEqual,
        eAlways
    };

    enum class PolygonMode
    {
        eFill,
        eLine,
        ePoint,
    };
    
} // namespace myNamespace