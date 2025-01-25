#pragma once
#include "SwiftStructs.hpp"

struct Scene;
struct Mesh;
namespace Parser
{
    void Init();
    std::vector<int> LoadMeshes(Scene& scene, std::filesystem::path filePath);
};  // namespace Parser
