#include "kyoto/AST/FunctionCall.h"

#include <fmt/core.h>
#include <stddef.h>
#include <stdexcept>
#include <utility>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/ModuleCompiler.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

FunctionCall::FunctionCall(std::string name, std::vector<ExpressionNode*> args, ModuleCompiler& compiler)
    : name(std::move(name))
    , args(std::move(args))
    , compiler(compiler)
{
}

FunctionCall::~FunctionCall()
{
    for (auto arg : args) {
        delete arg;
    }
}

std::string FunctionCall::to_string() const
{
    std::string str = name + "(";
    for (size_t i = 0; i < args.size(); i++) {
        str += args[i]->to_string();
        if (i != args.size() - 1) {
            str += ", ";
        }
    }
    str += ")";
    return str;
}

llvm::Value* FunctionCall::gen()
{
    std::vector<llvm::Value*> arg_values;
    for (auto& arg : args) {
        arg_values.push_back(arg->gen());
    }
    auto* fn = compiler.get_module()->getFunction(name);
    if (!fn) {
        throw std::runtime_error(fmt::format("Function `{}` not found", name));
    }
    return compiler.get_builder().CreateCall(fn, arg_values);
}

llvm::Type* FunctionCall::get_type(llvm::LLVMContext& context) const
{
    auto* fn = compiler.get_module()->getFunction(name);
    if (!fn) {
        throw std::runtime_error(fmt::format("Function `{}` not found", name));
    }
    return fn->getReturnType();
}

llvm::Value* FunctionCall::trivial_gen()
{
    throw std::runtime_error("Function call is not trivially evaluable");
    return nullptr;
}

bool FunctionCall::is_trivially_evaluable() const
{
    return false;
}
