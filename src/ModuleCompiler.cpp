#include "kyoto/ModuleCompiler.h"

#include "KyotoLexer.h"
#include "KyotoParser.h"
#include "antlr4-runtime.h"

ModuleCompiler::ModuleCompiler(const std::string& name, const std::string& code)
    : name(name)
    , code(code)
    , builder(context)
    , module(std::make_unique<llvm::Module>(name, context))
{
}

void ModuleCompiler::compile()
{
    antlr4::ANTLRInputStream input(code);
    kyoto::KyotoLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    tokens.fill();

    kyoto::KyotoParser parser(&tokens);
    auto* tree = parser.program();

    ASTBuilderVisitor visitor(*this);

    auto* program = std::any_cast<ASTNode*>(visitor.visit(tree));

    program->gen();

    llvm::verifyModule(*module);
    module->print(llvm::errs(), nullptr);
}