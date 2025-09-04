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
#include <llvm/IR/Value.h>
#include <stddef.h>
#include <stdexcept>

#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"

NewArrayNode::NewArrayNode(KType* type, ExpressionNode* size_expr, ModuleCompiler& compiler)
    : type(type)
    , size_expr(size_expr)
    , compiler(compiler)
{
    // For heap arrays, new T[n] returns T*, not T[n]
    generated_type = new PointerType(type->copy());
}

NewArrayNode::~NewArrayNode()
{
    delete generated_type;
    delete size_expr;
}

std::string NewArrayNode::to_string() const
{
    return std::format("new {}[{}]", type->to_string(), size_expr->to_string());
}

llvm::Value* NewArrayNode::gen()
{
    KType* size_type = size_expr->get_ktype();
    if (!size_type->is_primitive() || !size_type->as<PrimitiveType>()->is_integer()) {
        throw std::runtime_error(
            std::format("Array size expression must be of integer type, got: {}", size_type->to_string()));
    }

    llvm::Value* size_value = size_expr->gen();

    size_t element_size;
    if (type->is_primitive()) {
        element_size = type->as<PrimitiveType>()->width();
    } else if (type->is_class()) {
        element_size = compiler.get_type_size(type->get_class_name());
    } else {
        throw std::runtime_error(std::format("Unsupported type for new array: {}", type->to_string()));
    }

    llvm::Value* size_i64;
    if (size_value->getType()->isIntegerTy(64)) {
        size_i64 = size_value;
    } else {
        size_i64
            = compiler.get_builder().CreateIntCast(size_value, llvm::Type::getInt64Ty(compiler.get_context()), true);
    }

    llvm::Value* element_size_value
        = llvm::ConstantInt::get(llvm::Type::getInt64Ty(compiler.get_context()), element_size);
    llvm::Value* total_size = compiler.get_builder().CreateMul(size_i64, element_size_value);

    auto* malloc_fn = compiler.get_module()->getFunction("malloc");
    auto* ptr = compiler.get_builder().CreateCall(malloc_fn, total_size);
    return ptr;
}

llvm::Value* NewArrayNode::gen_ptr() const
{
    KType* size_type = size_expr->get_ktype();
    if (!size_type->is_primitive() || !size_type->as<PrimitiveType>()->is_integer()) {
        throw std::runtime_error(
            std::format("Array size expression must be of integer type, got: {}", size_type->to_string()));
    }

    llvm::Value* size_value = size_expr->gen();

    size_t element_size;
    if (type->is_primitive()) {
        element_size = type->as<PrimitiveType>()->width();
    } else if (type->is_class()) {
        element_size = compiler.get_type_size(type->get_class_name());
    } else {
        throw std::runtime_error(std::format("Unsupported type for new array: {}", type->to_string()));
    }

    llvm::Value* size_i64;
    if (size_value->getType()->isIntegerTy(64)) {
        size_i64 = size_value;
    } else {
        size_i64
            = compiler.get_builder().CreateIntCast(size_value, llvm::Type::getInt64Ty(compiler.get_context()), true);
    }

    llvm::Value* element_size_value
        = llvm::ConstantInt::get(llvm::Type::getInt64Ty(compiler.get_context()), element_size);
    llvm::Value* total_size = compiler.get_builder().CreateMul(size_i64, element_size_value);

    auto* malloc_fn = compiler.get_module()->getFunction("malloc");
    auto* ptr = compiler.get_builder().CreateCall(malloc_fn, total_size);
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