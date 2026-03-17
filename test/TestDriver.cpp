#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <optional>
#include <stdint.h>
#include <string>

#include "kyoto/ModuleCompiler.h"
#include "kyoto/utils/File.h"
#include "kyoto/utils/Test.h"

namespace utils {

void test_driver(const utils::TestCase& test_case)
{
    if (test_case.skip()) {
        GTEST_SKIP();
    }

    std::optional<std::filesystem::path> temp_entry_path;
    auto source_dir = test_case.source().parent_path();
    auto source_name = test_case.source().filename().string();

    if (!test_case.source().empty()) {
        const auto unique = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        temp_entry_path = source_dir / (".tmp-" + source_name + "-" + unique + ".kyo");

        std::ofstream out(*temp_entry_path);
        if (!out.is_open()) {
            throw std::runtime_error("Failed to create temporary Kyoto test file");
        }
        out << test_case.code();
    }

    ModuleCompiler compiler(test_case.code(), "main", temp_entry_path);
    auto ir = compiler.gen_ir();

    if (temp_entry_path.has_value()) {
        std::filesystem::remove(*temp_entry_path);
    }

    if (test_case.err()) {
        EXPECT_FALSE(ir.has_value());
    } else {
        EXPECT_TRUE(ir.has_value());
        int32_t ret = utils::File::execute_ir(ir.value());
        EXPECT_EQ(ret, test_case.ret());
    }
}

}
