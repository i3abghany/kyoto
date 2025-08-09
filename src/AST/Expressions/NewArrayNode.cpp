#include "kyoto/AST/Expressions/NewArrayNode.h"

#include <format>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>

#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"

NewArrayNode::NewArrayNode(KType* type, size_t n, ModuleCompiler& compiler)
    : type(type)
    , n(n)
    , compiler(compiler)
{
    generated_type = new ArrayType(type, n);
}

NewArrayNode::~NewArrayNode()
{
    delete generated_type;
}

std::string NewArrayNode::to_string() const
{
    return std::format("new {}[{}]", type->get_class_name(), n);
}

llvm::Value* NewArrayNode::gen()
{
    const auto& class_name = type->to_string();
    const auto& class_size = compiler.get_type_size(class_name);
    const size_t array_size = n * class_size;

    auto* malloc_fn = compiler.get_module()->getFunction("malloc");

    llvm::Value* size_arg = llvm::ConstantInt::get(llvm::Type::getInt64Ty(compiler.get_context()), array_size);
    auto* ptr = compiler.get_builder().CreateCall(malloc_fn, size_arg);
    return ptr;
}

llvm::Value* NewArrayNode::gen_ptr() const
{
    const auto& class_name = type->to_string();
    const auto& class_size = compiler.get_type_size(class_name);
    const size_t array_size = n * class_size;

    auto* malloc_fn = compiler.get_module()->getFunction("malloc");

    llvm::Value* size_arg = llvm::ConstantInt::get(llvm::Type::getInt64Ty(compiler.get_context()), array_size);
    auto* ptr = compiler.get_builder().CreateCall(malloc_fn, size_arg);
    return ptr;
}

llvm::Type* NewArrayNode::gen_type() const
{
    if (type->is_class()) return llvm::PointerType::get(compiler.get_llvm_struct(type->get_class_name()), 0);

    return ASTNode::get_llvm_type(type, compiler);
}

KType* NewArrayNode::get_ktype() const
{
    return generated_type;
}

llvm::Value* NewArrayNode::trivial_gen()
{
    return nullptr;
}

bool NewArrayNode::is_trivially_evaluable() const
{
    return false;
}