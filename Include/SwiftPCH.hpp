#pragma once

#include "memory"
#include "string"
#include "fstream"
#include "unordered_map"
#include "vector"
#include "filesystem"
#include "ranges"
#include "algorithm"
#include "map"
#include "iostream"
#include "any"

#include "vulkan/vulkan.hpp"
#include "glm/glm.hpp"
#include "vk_mem_alloc.h"

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#undef max
#undef min
#undef CreateSemaphore

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

#define VK_ASSERT(x, message) \
assert(x == vk::Result::eSuccess && message)

