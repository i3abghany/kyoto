#include "kyoto/AST/Expressions/FunctionCallNode.h"

#include <assert.h>
#include <format>
#include <optional>
#include <stddef.h>
#include <stdexcept>
#include <utility>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/Expressions/ExpressionNode.h"
#include "kyoto/KType.h"
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
    for (const auto arg : args)
        delete arg;
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
    auto* fn = compiler.get_module()->getFunction(name);
    if (!fn) {
        throw std::runtime_error(std::format("Function `{}` not found", name));
    }
    std::vector<llvm::Value*> arg_values;
    if (destination && is_constructor_call()) arg_values.push_back(destination);

    for (auto& arg : args)
        arg_values.push_back(arg->gen());

    if (fn->arg_size() != arg_values.size() && (!fn->isVarArg() || fn->arg_size() > arg_values.size())) {
        throw std::runtime_error(
            std::format("Function `{}` expects {} arguments, got {}", name, fn->arg_size(), arg_values.size()));
    }
    return compiler.get_builder().CreateCall(fn, arg_values);
}

llvm::Value* FunctionCall::gen_ptr() const
{
    assert(get_ktype()->is_pointer() && "Function does not return a pointer");
    std::vector<llvm::Value*> arg_values;
    if (destination && is_constructor_call()) arg_values.push_back(destination);

    for (auto& arg : args)
        arg_values.push_back(arg->gen());

    auto* fn = compiler.get_module()->getFunction(name);
    if (!fn) {
        throw std::runtime_error(std::format("Function `{}` not found", name));
    }
    return compiler.get_builder().CreateCall(fn, arg_values);
}

llvm::Type* FunctionCall::gen_type() const
{
    const auto* fn = compiler.get_module()->getFunction(name);
    if (!fn) {
        throw std::runtime_error(std::format("Function `{}` not found", name));
    }
    return fn->getReturnType();
}

llvm::Value* FunctionCall::trivial_gen()
{
    assert(false && "Function call is not trivially evaluable");
}

KType* FunctionCall::get_ktype() const
{
    auto fn = compiler.get_function(name);
    if (fn.has_value()) return fn.value()->get_ret_type();
    throw std::runtime_error(std::format("Function `{}` not found", name));
}

bool FunctionCall::is_trivially_evaluable() const
{
    return false;
}

void FunctionCall::set_destination(llvm::Value* dest)
{
    if (destination) throw std::runtime_error("Destination already set");
    destination = dest;
}

void FunctionCall::insert_arg(ExpressionNode* node, size_t index)
{
    if (index == 0 && destination)
        throw std::runtime_error("Cannot insert argument at index 0 when destination is set");
    args.insert(args.begin() + index, node);
}
