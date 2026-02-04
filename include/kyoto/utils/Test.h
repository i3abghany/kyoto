#pragma once

#include <iostream>
#include <string>

#define DEFINE_KYOTO_TEST_SUITE(SuiteName, Path)                         \
    class SuiteName : public ::testing::TestWithParam<utils::TestCase> { \
    protected:                                                           \
        utils::TestCase test_case;                                       \
        void SetUp() override                                            \
        {                                                                \
            test_case = GetParam();                                      \
        }                                                                \
    };                                                                   \
    TEST_P(SuiteName, SuiteName)                                         \
    {                                                                    \
        utils::test_driver(test_case);                                   \
    }                                                                    \
                                                                         \
    static auto generate_test_cases()                                    \
    {                                                                    \
        const auto test_cases = utils::File::get_test_cases(Path);       \
        return test_cases;                                               \
    }                                                                    \
    INSTANTIATE_TEST_SUITE_P(SuiteName, SuiteName, testing::ValuesIn(generate_test_cases()));

namespace utils {

class TestCase {
public:
    TestCase() = default;
    TestCase(std::string name, std::string code, int32_t expected_return, bool error = false, bool skip = false)
        : test_name(std::move(name))
        , kyoto_code(std::move(code))
        , expected_return(std::move(expected_return))
        , error(error)
        , should_skip(skip)
    {
    }

    std::string name() const { return test_name; }
    std::string code() const { return kyoto_code; }
    int32_t ret() const { return expected_return; }
    bool err() const { return error; }
    bool skip() const { return should_skip; }

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
    bool should_skip {};
};

void test_driver(const TestCase& test_case);

}
