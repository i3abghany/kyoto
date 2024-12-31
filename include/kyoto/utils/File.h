#pragma once

#include <string>
#include <string_view>

namespace utils {

class File {
    File() = delete;
    ~File() = delete;

public:
    static std::string get_source(const std::string_view filename);
};

}