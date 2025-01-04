#include <gtest/gtest-message.h>
#include <gtest/gtest-test-part.h>
#include <gtest/gtest_pred_impl.h>
#include <stdint.h>
#include <optional>
#include <string>

#include "kyoto/utils/Test.h"
#include "kyoto/ModuleCompiler.h"
#include "kyoto/utils/File.h"

namespace utils {

void test_driver(const utils::TestCase& test_case)
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

}