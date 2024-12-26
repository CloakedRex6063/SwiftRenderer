#pragma once

namespace Swift::Vulkan::Constants
{
    constexpr u16 MaxSamplerDescriptors = std::numeric_limits<u16>::max();
    constexpr u8 SamplerBinding = 0;
    constexpr u16 MaxUniformDescriptors = std::numeric_limits<u16>::max();
    constexpr u8 UniformBinding = 1;
    constexpr u16 MaxStorageDescriptors = std::numeric_limits<u16>::max();
    constexpr u8 StorageBinding = 2;
    constexpr u16 MaxImageDescriptors = std::numeric_limits<u16>::max();
    constexpr u8 ImageBinding = 3;
};