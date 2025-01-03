#include "kyoto/ModuleCompiler.h"

#include <any>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <utility>

#include "ANTLRInputStream.h"
#include "CommonTokenStream.h"
#include "KyotoLexer.h"
#include "KyotoParser.h"
#include "kyoto/AST/ASTNode.h"
#include "kyoto/Analysis/FunctionTermination.h"
#include "kyoto/Visitor.h"
#include "llvm/ADT/APInt.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/raw_ostream.h"

class KType;

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

void ModuleCompiler::llvm_pass()
{
    // We are only using the FunctionAnalysisManager but apparently we need to
    // register all of those managers.
    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;
    llvm::PassBuilder PB;

    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    llvm::FunctionPassManager FPM;
    FPM.addPass(FunctionTerminationPass(*this));

    llvm::ModulePassManager MPM;
    MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM)));

    MPM.run(*module, MAM);
}

void ModuleCompiler::ensure_main_fn()
{
    auto* main_fn = module->getFunction("main");
    if (!main_fn) {
        std::cerr << "Couldn't find main function\n";
        std::exit(1);
    }

    if (!main_fn->getReturnType()->isIntegerTy(32)) {
        main_fn->getReturnType()->dump();
        std::cerr << "Error: main function must return an i32\n";
        std::exit(1);
    }

    auto* last_bb = &main_fn->back();
    auto* term = last_bb->getTerminator();
    if (!term) {
        builder.SetInsertPoint(last_bb);
        builder.CreateRet(llvm::ConstantInt::get(context, llvm::APInt(32, 0)));
    }
}

void ModuleCompiler::insert_dummy_return(llvm::BasicBlock& bb)
{
    auto* llvm_type = bb.getParent()->getReturnType();

    if (!llvm_type) {
        return;
    }

    builder.SetInsertPoint(&bb);

    if (llvm_type->isVoidTy()) {
        builder.CreateRetVoid();
    } else if (llvm_type->isIntegerTy()) {
        builder.CreateRet(llvm::ConstantInt::get(context, llvm::APInt(llvm_type->getIntegerBitWidth(), 0)));
    } else {
        std::cerr << "Error: unsupported return type\n";
        std::exit(1);
    }
}

std::optional<std::string> ModuleCompiler::gen_ir()
{
    auto program = std::unique_ptr<ASTNode>(parse_program());
    try {
        program->gen();
    } catch (const std::exception& e) {
        constexpr const char* RED = "\033[0;31m";
        constexpr const char* NC = "\033[0m";
        std::cerr << RED << "Error: " << NC << e.what() << std::endl;
        return std::nullopt;
    }

    llvm_pass();

    if (!fn_termination_error) {
        ensure_main_fn();
    }

    std::string llvm_err;
    llvm::raw_string_ostream err(llvm_err);
    if (!verify_module(err)) {
        std::cerr << "Error: " << err.str() << std::endl;
        return std::nullopt;
    } else {
        std::string llvm_ir;
        llvm::raw_string_ostream os(llvm_ir);
        module->print(os, nullptr);
        return os.str();
    }

    return std::nullopt;
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

void ModuleCompiler::push_fn_return_type(KType* type)
{
    curr_fn_ret_type = type;
}

void ModuleCompiler::pop_fn_return_type()
{
    curr_fn_ret_type = nullptr;
}

KType* ModuleCompiler::get_fn_return_type() const
{
    return curr_fn_ret_type;
}

void ModuleCompiler::set_fn_termination_error()
{
    fn_termination_error = true;
}

size_t ModuleCompiler::n_scopes() const
{
    return symbol_table.n_scopes();
}

bool ModuleCompiler::verify_module(llvm::raw_string_ostream& os) const
{
    auto err = llvm::verifyModule(*module, &os);
    os.flush();
    return err == false;
}
