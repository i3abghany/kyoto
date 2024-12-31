#include <fstream>
#include <optional>
#include <string>

#include "kyoto/ModuleCompiler.h"
#include "kyoto/utils/File.h"
#include "support/Declarations.h"

using namespace antlr4;

int main()
{
    const auto source = utils::File::get_source("../examples/ex.kyo");
    ModuleCompiler compiler(source);
    auto ir = compiler.gen_ir();
    if (ir) {
        std::ofstream ofs("ex.ll");
        ofs << *ir;
    }

    return 0;
}
