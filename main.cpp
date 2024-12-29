#include <fstream>
#include <iostream>
#include <string_view>

#include "KyotoLexer.h"
#include "KyotoParser.h"
#include "antlr4-runtime.h"

using namespace antlrcpptest;
using namespace antlr4;

std::string get_source(std::string_view filename)
{
    std::ifstream ifs(filename.data());
    if (!ifs.is_open()) {
        throw std::runtime_error("Failed to open file");
    }
    return std::string((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
}

int main(int, const char**)
{
    auto source = get_source("../examples/ex.kyo");
    ANTLRInputStream input(source);
    KyotoLexer lexer(&input);
    CommonTokenStream tokens(&lexer);

    tokens.fill();
    for (auto token : tokens.getTokens()) {
        std::cout << token->toString() << '\n';
    }

    KyotoParser parser(&tokens);
    auto* tree = parser.program();
    std::cout << std::endl << tree->toStringTree(&parser) << std::endl << std::endl;

    return 0;
}
