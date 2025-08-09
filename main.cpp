#include <boost/program_options.hpp>
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
    desc.add_options()("help,h", "Print this help message")("run,r", "Run the program in `lli` after compilation")(
        "output,o", po::value<std::string>()->default_value("a.out"), "Output file for the executable binary");

    po::positional_options_description pos;
    pos.add("files", -1);

    po::options_description hidden("Hidden options");
    hidden.add_options()("files", po::value<std::vector<std::string>>()->multitoken(), "Positional input files");

    po::options_description all;
    all.add(desc).add(hidden);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(all).positional(pos).run(), vm);
    po::notify(vm);

    if (vm.contains("help")) {
        print_usage(argv[0], desc);
        return 0;
    }

    if (!vm.contains("files")) {
        constexpr auto* RED = "\033[0;31m";
        constexpr auto* NC = "\033[0m";
        std::cerr << RED << "Error: " << NC << "No input files provided" << std::endl;
        return 1;
    }

    auto files = vm["files"].as<std::vector<std::string>>();
    if (files.size() > 1) {
        std::cerr << "Only one input file is supported" << std::endl;
        return 1;
    }

    const auto& file = files[0];
    auto source = utils::File::get_source(file);
    ModuleCompiler compiler(source);

    auto output = vm["output"].as<std::string>();
    auto ir = compiler.gen_ir();

    if (!ir) return 1;

    if (vm.contains("run")) {
        return utils::File::execute_ir(*ir);
    }

    if (!utils::File::compile_ir_to_binary(*ir, output)) {
        std::cerr << "Error: Failed to compile to binary" << std::endl;
        return 1;
    }

    return 0;
}

int main(int argc, const char* argv[])
{
    return run(argc, argv);
}
