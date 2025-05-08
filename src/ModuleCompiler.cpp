#include "kyoto/ModuleCompiler.h"

#include <any>
#include <assert.h>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <llvm/IR/IRBuilder.h>
#include <stdexcept>
#include <utility>
#include <vector>

#include "ANTLRInputStream.h"
#include "CommonTokenStream.h"
#include "KyotoLexer.h"
#include "KyotoParser.h"
#include "kyoto/AST/ASTNode.h"
#include "kyoto/Analysis/FunctionTermination.h"
#include "kyoto/KType.h"
#include "kyoto/Resolution/ClassIdentifierVisitor.h"
#include "kyoto/Resolution/ConstructorIdentifierVisitor.h"
#include "kyoto/Resolution/FunctionIdentifierVisitor.h"
#include "kyoto/Visitor.h"
#include "llvm/ADT/APInt.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/raw_ostream.h"

ModuleCompiler::ModuleCompiler(const std::string& code, const std::string& name)
    : code(code)
    , name(name)
    , builder(context)
    , module(std::make_unique<llvm::Module>(name, context))
{
    register_visitors();
    register_malloc();
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

void ModuleCompiler::ensure_main_fn() const
{
    const auto* main_fn = module->getFunction("main");
    if (!main_fn) {
        throw std::runtime_error("Main function not found");
    }

    if (!main_fn->getReturnType()->isIntegerTy(32)) {
        main_fn->getReturnType()->dump();
        throw std::runtime_error("Main function must return an `i32`");
    }
}

void ModuleCompiler::insert_dummy_return(llvm::BasicBlock& bb)
{
    const auto* llvm_type = bb.getParent()->getReturnType();

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

llvm::BasicBlock* ModuleCompiler::create_basic_block(const std::string& name)
{
    return llvm::BasicBlock::Create(context, name, builder.GetInsertBlock()->getParent());
}

std::optional<std::string> ModuleCompiler::gen_ir()
{
    auto report_error = [](const std::string& msg) {
        constexpr auto RED = "\033[0;31m";
        constexpr auto NC = "\033[0m";
        std::cerr << RED << "Error: " << NC << msg << std::endl;
    };

    try {
        auto program = std::unique_ptr<ASTNode>(parse_program());

        for (auto& visitor : analysis_visitors)
            visitor->visit(program.get());

        program->gen();
        llvm_pass();
        ensure_main_fn();
    } catch (const std::exception& e) {
        report_error(e.what());
        return std::nullopt;
    }

    std::string llvm_err;
    llvm::raw_string_ostream err(llvm_err);
    if (!verify_module(err)) {
        std::cerr << "Error: " << err.str() << std::endl;
        return std::nullopt;
    }

    std::string llvm_ir;
    llvm::raw_string_ostream os(llvm_ir);
    module->print(os, nullptr);
    return os.str();
}

std::optional<Symbol> ModuleCompiler::get_symbol(const std::string& name)
{
    return symbol_table.get_symbol(name);
}

void ModuleCompiler::add_symbol(const std::string& name, Symbol symbol)
{
    symbol_table.add_symbol(name, symbol);
}
void ModuleCompiler::add_function(FunctionNode* node)
{
    functions[node->get_name()] = node;
}

std::optional<FunctionNode*> ModuleCompiler::get_function(const std::string& name)
{
    if (functions.contains(name)) return functions[name];
    return std::nullopt;
}

void ModuleCompiler::register_malloc()
{
    auto* ret_type = new PointerType(new PrimitiveType(PrimitiveType::Kind::I8));
    FunctionNode::Parameter param;
    param.name = "size";
    param.type = new PrimitiveType(PrimitiveType::Kind::I32);
    std::vector<FunctionNode::Parameter> args = { param };
    auto* malloc = new FunctionNode("malloc", args, false, ret_type, nullptr, *this);
    add_function(malloc);
    (void)malloc->gen_prototype();
}

void ModuleCompiler::register_visitors()
{
    analysis_visitors.push_back(std::make_unique<ConstructorIdentifierVisitor>(*this));
    analysis_visitors.push_back(std::make_unique<ClassIdentifierVisitor>(*this));
    analysis_visitors.push_back(std::make_unique<FunctionIdentifierVisitor>(*this));
}

void ModuleCompiler::push_scope()
{
    symbol_table.push_scope();

    // We insert function arguments into the symbol table. This is once we are
    // in the second scope as the first scope is the global scope.
    if (symbol_table.n_scopes() == 2) {
        int i = 0;
        for (auto iter = current_fn->arg_begin(); iter != current_fn->arg_end(); iter++) {
            llvm::Value* arg = &*iter;
            auto arg_name = current_fn_node->get_params()[i++].name;
            auto* arg_type = arg->getType();
            auto* arg_alloc = builder.CreateAlloca(arg_type, nullptr, arg_name);
            builder.CreateStore(arg, arg_alloc);
            symbol_table.add_symbol(arg_name, Symbol { arg_alloc, current_fn_node->get_params()[i - 1].type->copy() });
        }
    }
}

void ModuleCompiler::pop_scope()
{
    // destruct the types of the function arguments. `2` is the global scope and
    // the function scope. Whene we pop the function, we destruct the types of
    // the function arguments.
    if (symbol_table.n_scopes() == 2) {
        int i = 0;
        for (auto iter = current_fn->arg_begin(); iter != current_fn->arg_end(); iter++) {
            auto arg_name = current_fn_node->get_params()[i++].name;
            auto s = symbol_table.get_symbol(arg_name);
            assert(s.has_value() && "Expected function parameter symbol to be in the symbol table");
            delete s.value().type;
        }
    }

    symbol_table.pop_scope();
}

FunctionNode* ModuleCompiler::get_current_function_node() const
{
    return current_fn_node;
}

void ModuleCompiler::set_current_function(FunctionNode* node, llvm::Function* func)
{
    current_fn_node = node;
    current_fn = func;
}

void ModuleCompiler::push_class(std::string name)
{
    current_class = std::move(name);
    classes.insert(current_class);
}

void ModuleCompiler::pop_class()
{
    current_class = "";
}

std::string ModuleCompiler::get_current_class() const
{
    return current_class;
}

bool ModuleCompiler::class_exists(const std::string& name) const
{
    return classes.contains(name);
}

void ModuleCompiler::add_class_metadata(const std::string& name, const ClassMetadata& data)
{
    classes_metadata[name] = data;
}

llvm::StructType* ModuleCompiler::get_llvm_struct(const std::string& name) const
{
    return classes_metadata.at(name).llvm_type;
}

ClassMetadata& ModuleCompiler::get_class_metadata(const std::string& name)
{
    return classes_metadata.at(name);
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
