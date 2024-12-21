#pragma once

class FileIO 
{
public:
    static std::vector<char> ReadBinaryFile(const std::string_view filePath);
};
