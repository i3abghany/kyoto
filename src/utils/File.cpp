#include "kyoto/utils/File.h"

#include <fmt/core.h>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace utils {

std::string File::get_source(const std::string_view filename)
{
    std::ifstream ifs(filename.data());
    if (!ifs.is_open()) {
        throw std::runtime_error(fmt::format("Failed to open file: {}", filename));
    }
    return std::string { std::istreambuf_iterator<char>(ifs), {} };
}

}