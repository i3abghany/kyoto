#pragma once

#include <stdint.h>
#include <string>
#include <string_view>
#include <vector>

namespace utils {

class TestCase;

class File {
    File() = delete;
    ~File() = delete;

public:
    static std::string get_source(const std::string_view filename);
    static std::vector<TestCase> get_test_cases(const std::string_view filename);
    static int32_t execute_ir(const std::string& ir);

private:
    static std::vector<TestCase> split_test_cases(const std::string& source);
    static TestCase parse_test_case(const std::string& test_case);
    static void trim(std::string& str);
};

}