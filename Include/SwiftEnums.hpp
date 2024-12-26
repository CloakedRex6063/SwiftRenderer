#pragma once

enum class ImageFormat : uint8_t
{
    eR16G16B16A16_SRGB,
    eR32G32B32A32_SRGB,
};

enum class BufferType : uint8_t
{
    eUniform,
    eStorage,
    eIndex
};