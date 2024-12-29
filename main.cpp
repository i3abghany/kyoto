#include <fstream>
#include <iostream>
#include <string_view>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/ModuleCompiler.h"
#include "kyoto/Visitor.h"

using namespace antlr4;

std::string get_source(std::string_view filename)
{
    std::ifstream ifs(filename.data());
    if (!ifs.is_open()) {
        throw std::runtime_error("Failed to open file");
    }
    return std::string((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
}

int main()
{
    auto source = get_source("../examples/ex.kyo");
    ModuleCompiler compiler("main", source);
    compiler.compile();
    return 0;
}
