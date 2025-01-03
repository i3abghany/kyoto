#include <boost/any.hpp>
#include <boost/program_options/detail/parsers.hpp>
#include <boost/program_options/detail/value_semantic.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/type_index/type_index_facade.hpp>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "kyoto/ModuleCompiler.h"
#include "kyoto/utils/File.h"
#include "support/Declarations.h"

using namespace antlr4;
namespace po = boost::program_options;

void print_usage(const std::string& prog, const po::options_description& desc)
{
    std::cout << "Usage: " << prog << " [OPTIONS] INPUT_FILE" << std::endl;
    std::cout << desc << std::endl;
}

int run(int argc, const char* argv[])
{
    po::options_description desc("The Kyoto Programming Language Compiler");
    desc.add_options()("help,h", "Print this help message")(
        "output,o", po::value<std::string>()->default_value("out.ll"), "Output file for the LLVM IR");

    po::positional_options_description pos;
    pos.add("files", -1);

    po::options_description hidden("Hidden options");
    hidden.add_options()("files", po::value<std::vector<std::string>>()->multitoken(), "Positional input files");

    po::options_description all;
    all.add(desc).add(hidden);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(all).positional(pos).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        print_usage(argv[0], desc);
        return 0;
    }

    if (!vm.count("files")) {
        constexpr const char* RED = "\033[0;31m";
        constexpr const char* NC = "\033[0m";
        std::cerr << RED << "Error: " << NC << "No input files provided" << std::endl;
        return 1;
    }

    auto files = vm["files"].as<std::vector<std::string>>();
    if (files.size() > 1) {
        std::cerr << "Only one input file is supported" << std::endl;
        return 1;
    }

    auto file = files[0];
    auto source = utils::File::get_source(file);
    ModuleCompiler compiler(source);

    auto output = vm["output"].as<std::string>();
    auto ir = compiler.gen_ir();
    if (ir) {
        std::ofstream ofs(output);
        ofs << *ir;
        return 0;
    }

    return 1;
}

int main(int argc, const char* argv[])
{
    return run(argc, argv);
}
