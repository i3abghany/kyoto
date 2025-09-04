#include "kyoto/utils/File.h"

#include <assert.h>
#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/sequence/intrinsic/at_key.hpp>
#include <boost/process/child.hpp>
#include <boost/process/detail/child_decl.hpp>
#include <boost/process/io.hpp>
#include <boost/process/pipe.hpp>
#include <boost/process/search_path.hpp>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <stddef.h>
#include <stdexcept>
#include <utility>

#include "kyoto/utils/Test.h"

namespace utils {

std::string File::get_source(const std::string_view filename)
{
    std::ifstream ifs(filename.data());
    if (!ifs.is_open()) {
        throw std::runtime_error(std::format("Failed to open file: {}", filename));
    }
    return std::string { std::istreambuf_iterator(ifs), {} };
}

std::vector<TestCase> File::get_test_cases(const std::string_view filename)
{
    const auto source = get_source(filename);
    return split_test_cases(source);
}

int32_t File::execute_ir(const std::string& ir)
{
    auto temp_file = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path("temp-%%%%-%%%%.ll");
    std::ofstream ofs(temp_file.string());
    if (!ofs.is_open()) {
        throw std::runtime_error("Failed to open temporary file");
    }
    ofs << ir;
    ofs.close();

    if (!is_executable("lli")) {
        throw std::runtime_error("Could not find `lli` in PATH");
    }

    boost::process::ipstream error_stream;
    boost::process::child proc { "lli " + temp_file.string(), boost::process::std_err > error_stream };
    proc.wait();
    int exit_code = proc.exit_code();
    boost::filesystem::remove(temp_file);
    return ((exit_code & 0xFF) << 24) >> 24;
}

bool File::compile_ir_to_binary(const std::string& ir, const std::string& output_path)
{
    auto temp_ir_file = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path("temp-%%%%-%%%%.ll");
    std::ofstream ofs(temp_ir_file.string());
    if (!ofs.is_open()) {
        std::cerr << "Error: Failed to create temporary IR file" << std::endl;
        return false;
    }
    ofs << ir;
    ofs.close();

    if (is_executable("clang")) {
        std::string cmd = "clang -O2 " + temp_ir_file.string() + " -o " + output_path;
        boost::process::child proc { cmd };
        proc.wait();
        boost::filesystem::remove(temp_ir_file);
        return proc.exit_code() == 0;
    }

    if (is_executable("llc") && is_executable("gcc")) {
        auto temp_asm_file
            = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path("temp-%%%%-%%%%.s");
        std::string llc_cmd = "llc -O2 " + temp_ir_file.string() + " -o " + temp_asm_file.string();
        boost::process::child llc_proc { llc_cmd };
        llc_proc.wait();

        if (llc_proc.exit_code() != 0) {
            boost::filesystem::remove(temp_ir_file);
            return false;
        }

        std::string gcc_cmd = "gcc " + temp_asm_file.string() + " -o " + output_path;
        boost::process::child gcc_proc { gcc_cmd };
        gcc_proc.wait();

        boost::filesystem::remove(temp_ir_file);
        boost::filesystem::remove(temp_asm_file);
        return gcc_proc.exit_code() == 0;
    }

    boost::filesystem::remove(temp_ir_file);
    std::cerr << "Error: No suitable compiler found (`clang` or `llc`+`gcc` required)" << std::endl;
    return false;
}

bool File::is_executable(const std::string& name)
{
    const auto path = boost::process::search_path(name);
    return !path.empty();
}

std::vector<TestCase> File::split_test_cases(const std::string& source)
{
    std::vector<TestCase> test_cases;
    size_t start = 0;
    const char* data = source.data();
    constexpr const char* delim = "// NAME";
    size_t delim_pos = 0;
    while ((delim_pos = source.find(delim, start + 1)) != std::string::npos) {
        std::string test_case = { data + start, delim_pos - start };
        boost::trim(test_case);
        test_cases.emplace_back(parse_test_case(test_case));
        start = delim_pos;
    }

    std::string test_case = { data + start, source.size() - start };
    boost::trim(test_case);
    test_cases.emplace_back(parse_test_case(test_case));
    return test_cases;
}

TestCase File::parse_test_case(const std::string& test_case)
{
    std::string code;
    int32_t expected_return = 0;
    bool error = false;
    bool skip = false;
    std::string line;
    std::istringstream iss(test_case);

    std::getline(iss, line);
    assert(line.starts_with("// NAME "));
    const std::string name = line.substr(7);

    std::getline(iss, line);
    assert(line.starts_with("// ERR "));
    error = std::stoi(line.substr(6)) == 1;

    std::getline(iss, line);
    assert(line.starts_with("// RET "));
    expected_return = std::stoi(line.substr(6));

    std::getline(iss, line);
    if (line.starts_with("// SKIP")) skip = true;
    else code = line;

    code += std::string { std::istreambuf_iterator<char>(iss), {} };
    return TestCase(name, code, expected_return, error, skip);
}

}
