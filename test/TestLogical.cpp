#include <gtest/gtest-message.h>
#include <gtest/gtest-param-test.h>
#include <gtest/gtest-test-part.h>
#include <gtest/gtest_pred_impl.h>
#include <optional>
#include <stdint.h>
#include <string>
#include <vector>

#include "kyoto/ModuleCompiler.h"
#include "kyoto/utils/File.h"
#include "kyoto/utils/Test.h"

class TestLogical : public ::testing::TestWithParam<utils::TestCase> {
protected:
    utils::TestCase test_case;

    void SetUp() override { test_case = GetParam(); }
};

TEST_P(TestLogical, TestLogical)
{
    if (test_case.skip()) {
        GTEST_SKIP();
    }

    ModuleCompiler compiler(test_case.code());
    auto ir = compiler.gen_ir();

    if (test_case.err()) {
        EXPECT_FALSE(ir.has_value());
    } else {
        EXPECT_TRUE(ir.has_value());
        int32_t ret = utils::File::execute_ir(ir.value());
        EXPECT_EQ(ret, test_case.ret());
    }
}

static auto generate_test_cases()
{
    const auto test_cases = utils::File::get_test_cases("../test/code/logical.kyo");
    return test_cases;
}

INSTANTIATE_TEST_SUITE_P(TestLogical, TestLogical, testing::ValuesIn(generate_test_cases()));