#include "kyoto/AST/NumberNode.h"

#include <assert.h>
#include <fmt/core.h>
#include <iostream>
#include <stddef.h>

#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "llvm/ADT/APInt.h"
#include "llvm/IR/Constants.h"

NumberNode::NumberNode(int64_t value, KType* ktype, ModuleCompiler& compiler)
    : value(value)
    , type(ktype)
    , compiler(compiler)
{
}

NumberNode::~NumberNode()
{
    delete type;
}

std::string NumberNode::to_string() const
{
    return fmt::format("{}({})", type->to_string(), value);
}

llvm::Value* NumberNode::gen()
{
    auto primitive_type = dynamic_cast<PrimitiveType*>(type);
    assert(primitive_type && "NumberNode type must be a primitive type");
    size_t width = primitive_type->width();
    auto b = primitive_type->is_boolean();
    return llvm::ConstantInt::get(compiler.get_context(), llvm::APInt(b ? 1 : width * 8, value, b ? false : true));
}

llvm::Type* NumberNode::get_type(llvm::LLVMContext& context) const
{
    return ASTNode::get_llvm_type(type, context);
}

bool NumberNode::is_trivially_evaluable() const
{
    return true;
}

llvm::Value* NumberNode::trivial_gen()
{
    auto primitive_type = dynamic_cast<PrimitiveType*>(type);
    assert(primitive_type && "NumberNode type must be a primitive type");
    // return the maximum-width integer constant
    return llvm::ConstantInt::get(compiler.get_context(), llvm::APInt(64, value, true));
}

void NumberNode::cast_to(PrimitiveType::Kind target_kind)
{
    auto target_type = new PrimitiveType(target_kind);
    delete type;
    type = target_type;
}