#include "kyoto/AST/NumberNode.h"

#include <assert.h>
#include <fmt/core.h>
#include <stddef.h>

#include "kyoto/ModuleCompiler.h"
#include "llvm/ADT/APInt.h"
#include "llvm/IR/Constants.h"

NumberNode::NumberNode(int64_t value, KType* ktype, ModuleCompiler& compiler)
    : value(value)
    , type(ktype)
    , compiler(compiler)
{
}

std::string NumberNode::to_string() const
{
    return fmt::format("{}Node({})", type->to_string(), value);
}

llvm::Value* NumberNode::gen()
{
    auto primitive_type = dynamic_cast<PrimitiveType*>(type.get());
    assert(primitive_type && "NumberNode type must be a primitive type");
    size_t width = primitive_type->width();
    return llvm::ConstantInt::get(compiler.get_context(), llvm::APInt(width * 8, value, true));
}

llvm::Type* NumberNode::get_type(llvm::LLVMContext& context) const
{
    return ASTNode::get_llvm_type(type.get(), context);
}