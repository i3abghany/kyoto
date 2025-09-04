#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "kyoto/utils/File.h"
#include "kyoto/utils/Test.h"

class TestCast : public ::testing::TestWithParam<utils::TestCase> {
protected:
    utils::TestCase test_case;

    void SetUp() override { test_case = GetParam(); }
};

TEST_P(TestCast, TestCast)
{
    utils::test_driver(test_case);
}

static auto generate_test_cases()
{
    const auto test_cases = utils::File::get_test_cases("../test/code/cast.kyo");
    return test_cases;
}

INSTANTIATE_TEST_SUITE_P(TestCast, TestCast, testing::ValuesIn(generate_test_cases()));
