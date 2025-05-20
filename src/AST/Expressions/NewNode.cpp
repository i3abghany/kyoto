#include "kyoto/AST/Expressions/NewNode.h"

#include <fmt/core.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>

#include "kyoto/AST/Expressions/FunctionCallNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"

NewNode::NewNode(KType* type, FunctionCall* constructor_call, ModuleCompiler& compiler)
    : type(type)
    , constructor_call(constructor_call)
    , compiler(compiler)
{
    generated_type = new PointerType(type);
}

NewNode::~NewNode()
{
    delete generated_type;
}

std::string NewNode::to_string() const
{
    return fmt::format("New({})", constructor_call->to_string());
}

llvm::Value* NewNode::gen()
{
    const auto& class_name = type->get_class_name();
    const auto& class_size = compiler.get_class_size(class_name);
    auto* malloc_fn = compiler.get_module()->getFunction("malloc");

    llvm::ArrayRef<llvm::Value*> args = {
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(compiler.get_context()), class_size),
    };

    auto* ptr = compiler.get_builder().CreateCall(malloc_fn, args);
    constructor_call->set_destination(ptr);

    constructor_call->gen();
    return ptr;
}

llvm::Value* NewNode::gen_ptr() const
{
    const auto& class_name = type->get_class_name();
    const auto& class_size = compiler.get_class_size(class_name);
    auto* malloc_fn = compiler.get_module()->getFunction("malloc");

    llvm::ArrayRef<llvm::Value*> args = {
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(compiler.get_context()), class_size),
    };

    auto* ptr = compiler.get_builder().CreateCall(malloc_fn, args);
    constructor_call->set_destination(ptr);

    constructor_call->gen();
    return ptr;
}

llvm::Type* NewNode::gen_type() const
{
    return llvm::PointerType::get(compiler.get_llvm_struct(type->get_class_name()), 0);
}

KType* NewNode::get_ktype() const
{
    return generated_type;
}

llvm::Value* NewNode::trivial_gen()
{
    return nullptr;
}

bool NewNode::is_trivially_evaluable() const
{
    return false;
}