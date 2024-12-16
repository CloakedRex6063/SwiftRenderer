#pragma once
#include "vulkan/vulkan.hpp"

struct SwiftInit
{
    std::string appName{};
    std::string engineName{};
    vk::SurfaceKHR surface{};
};