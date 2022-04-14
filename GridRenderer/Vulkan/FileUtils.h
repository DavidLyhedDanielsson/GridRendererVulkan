#pragma once

#include <filesystem>
#include <optional>
#include <vector>

namespace FileUtils
{
    std::optional<std::vector<char>> readFile(const std::filesystem::path& path);
}