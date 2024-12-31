#include "kyoto/ModuleCompiler.h"

#include <any>
#include <iostream>
#include <utility>

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

ASTNode* ModuleCompiler::parse_program()
{
    antlr4::ANTLRInputStream input(code);
    kyoto::KyotoLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    tokens.fill();

    kyoto::KyotoParser parser(&tokens);
    auto* tree = parser.program();

    ASTBuilderVisitor visitor(*this);
    return std::any_cast<ASTNode*>(visitor.visit(tree));
}

std::optional<std::string> ModuleCompiler::gen_ir()
{
    auto program = parse_program();
    program->gen();
    if (verify_module()) {
        std::string str;
        llvm::raw_string_ostream os(str);
        module->print(os, nullptr);
        return os.str();
    } else {
        return std::nullopt;
    }
}

std::optional<Symbol> ModuleCompiler::get_symbol(const std::string& name)
{
    return symbol_table.get_symbol(name);
}

void ModuleCompiler::add_symbol(const std::string& name, Symbol symbol)
{
    symbol_table.add_symbol(name, std::move(symbol));
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

bool ModuleCompiler::verify_module() const
{
    auto err = llvm::verifyModule(*module, nullptr);
    if (err) {
        std::cerr << "Module verification failed" << std::endl;
        llvm::errs() << "Error(s) in module " << module->getName() << ":\n";
        llvm::verifyModule(*module, &llvm::errs());
    }
    return err == false;
}