#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "kyoto/utils/File.h"
#include "kyoto/utils/Test.h"

class TestIfStatement : public ::testing::TestWithParam<utils::TestCase> {
protected:
    utils::TestCase test_case;

    void SetUp() override { test_case = GetParam(); }
};

TEST_P(TestIfStatement, TestIfStatement)
{
    utils::test_driver(test_case);
}

static auto generate_test_cases()
{
    const auto test_cases = utils::File::get_test_cases("../test/code/if.kyo");
    return test_cases;
}

INSTANTIATE_TEST_SUITE_P(TestIfStatement, TestIfStatement, testing::ValuesIn(generate_test_cases()));