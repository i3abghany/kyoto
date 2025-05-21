#include "kyoto/AST/Expressions/NumberNode.h"

#include <fmt/core.h>
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
    if (!type->is_primitive()) throw std::runtime_error("NumberNode type must be a primitive type");
    const auto primitive_type = dynamic_cast<PrimitiveType*>(type);
    size_t width = primitive_type->width();
    auto b = primitive_type->is_boolean();
    return llvm::ConstantInt::get(compiler.get_context(), llvm::APInt(b ? 1 : width * 8, value, b ? false : true));
}

llvm::Type* NumberNode::gen_type() const
{
    return get_llvm_type(type, compiler);
}

bool NumberNode::is_trivially_evaluable() const
{
    return true;
}

llvm::Value* NumberNode::trivial_gen()
{
    if (!type->is_primitive()) throw std::runtime_error("NumberNode type must be a primitive type");
    auto primitive_type = dynamic_cast<PrimitiveType*>(type);
    return llvm::ConstantInt::get(compiler.get_context(), llvm::APInt(64, value, true));
}

KType* NumberNode::get_ktype() const
{
    return type;
}

void NumberNode::cast_to(PrimitiveType::Kind target_kind)
{
    const auto target_type = new PrimitiveType(target_kind);
    delete type;
    type = target_type;
}