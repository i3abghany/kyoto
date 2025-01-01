#pragma once

#include <string>

namespace utils {

class TestCase {
public:
    TestCase() = default;
    TestCase(std::string name, std::string code, int32_t expected_return, bool error = false)
        : test_name(std::move(name))
        , kyoto_code(std::move(code))
        , expected_return(std::move(expected_return))
        , error(error)
    {
    }

    std::string name() const { return test_name; }
    std::string code() const { return kyoto_code; }
    int32_t ret() const { return expected_return; }
    bool err() const { return error; }

    friend std::ostream& operator<<(std::ostream& os, const TestCase& test_case)
    {
        os << "Test: " << test_case.test_name << '\n';
        os << "Code: " << test_case.kyoto_code << '\n';
        os << "Expected return: " << test_case.expected_return << '\n';
        os << "Error: " << test_case.error << std::endl;
        return os;
    }

private:
    std::string test_name {};
    std::string kyoto_code {};
    int32_t expected_return {};
    bool error {};
};

}
