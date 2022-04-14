#include "FileUtils.h"

#include <cassert>
#include <fstream>
#include <limits>

namespace FileUtils
{
    std::optional<std::vector<char>> readFile(const std::filesystem::path& path)
    {
        std::ifstream in(path, std::ios::binary | std::ios::ate);
        if(!in.is_open())
            return std::nullopt;

        std::vector<char> outData(in.tellg());
        in.seekg(0);
        assert(outData.size() <= std::numeric_limits<std::streamsize>::max());
        if(outData.size() <= std::numeric_limits<std::streamsize>::max())
        {
            in.read(outData.data(), (std::streamsize)outData.size());
        }

        return outData;
    }
}
