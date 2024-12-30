#include "kyoto/ModuleCompiler.h"

#include <any>

#include "ANTLRInputStream.h"
#include "CommonTokenStream.h"
#include "KyotoLexer.h"
#include "KyotoParser.h"

#include "kyoto/AST/ASTNode.h"
#include "kyoto/Visitor.h"

#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

ModuleCompiler::ModuleCompiler(const std::string& code, const std::string& name)
    : code(code)
    , name(name)
    , builder(context)
    , module(std::make_unique<llvm::Module>(name, context))
    , symbol_table(*this, module.get())
    , type_resolver()
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

    verify_module();
}

std::optional<llvm::AllocaInst*> ModuleCompiler::get_symbol(const std::string& name)
{
    return symbol_table.get_symbol(name);
}

void ModuleCompiler::add_symbol(const std::string& name, llvm::AllocaInst* value)
{
    symbol_table.add_symbol(name, value);
}

void ModuleCompiler::push_scope()
{
    symbol_table.push_scope();
}

void ModuleCompiler::pop_scope()
{
    symbol_table.pop_scope();
}

size_t ModuleCompiler::n_scopes() const
{
    return symbol_table.n_scopes();
}

void ModuleCompiler::verify_module() const
{
    if (!verifyModule(*module, nullptr))
        std::cerr << "Module verification failed" << std::endl;
    module->print(llvm::errs(), nullptr);
}