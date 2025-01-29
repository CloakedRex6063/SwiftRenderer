#pragma once
#include "SwiftStructs.hpp"

struct Scene;
struct Mesh;

enum class ParserError
{
    eNoError,
    eFileNotFound,
    eInvalidFile,
};

namespace Parser
{
    void Init();
    std::expected<std::vector<int>, ParserError> LoadMeshes(Scene& scene, std::filesystem::path filePath);
};  // namespace Parser
