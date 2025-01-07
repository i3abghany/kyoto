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
#include <fmt/core.h>
#include <fstream>
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
        throw std::runtime_error(fmt::format("Failed to open file: {}", filename));
    }
    return std::string { std::istreambuf_iterator<char>(ifs), {} };
}

std::vector<TestCase> File::get_test_cases(const std::string_view filename)
{
    auto source = get_source(filename);
    return split_test_cases(source);
}

int32_t File::execute_ir(const std::string& ir)
{
    auto temp_file = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path("temp-%%%%-%%%%.ll");
    std::ofstream ofs(temp_file.string());
    assert(ofs.is_open());
    ofs << ir;
    ofs.close();

    boost::process::ipstream error_stream;
    boost::process::child proc { "lli " + temp_file.string(), boost::process::std_err > error_stream };
    proc.wait();
    int exit_code = proc.exit_code();
    boost::filesystem::remove(temp_file);
    return ((exit_code & 0xFF) << 24) >> 24;
}

std::vector<TestCase> File::split_test_cases(const std::string& source)
{
    std::vector<TestCase> test_cases;
    size_t start = 0;
    const char* data = source.data();
    constexpr const char* delim = "// SEP";
    constexpr size_t delim_len = 6;
    size_t delim_pos = 0;
    while ((delim_pos = source.find(delim, start)) != std::string::npos) {
        std::string test_case = { data + start, delim_pos - start };
        boost::trim(test_case);
        test_cases.emplace_back(parse_test_case(test_case));
        start = delim_pos + delim_len;
    }

    std::string test_case = { data + start, source.size() - start };
    boost::trim(test_case);
    test_cases.emplace_back(parse_test_case(test_case));
    return test_cases;
}

TestCase File::parse_test_case(const std::string& test_case)
{
    std::string name;
    std::string code;
    int32_t expected_return = 0;
    bool error = false;
    bool skip = false;
    std::string line;
    std::istringstream iss(test_case);

    std::getline(iss, line);
    assert(line.starts_with("// NAME "));
    name = line.substr(7);

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
