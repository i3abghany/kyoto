#include "kyoto/AST/StringLiteralNode.h"

#include <fmt/core.h>
#include <utility>

#include "kyoto/ModuleCompiler.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"

StringLiteralNode::StringLiteralNode(std::string value, ModuleCompiler& compiler)
    : value(std::move(value))
    , compiler(compiler)
{
}

StringLiteralNode::~StringLiteralNode() { }

std::string StringLiteralNode::to_string() const
{
    return fmt::format("StringLiteralNode(\"{}\")", value);
}

llvm::Value* StringLiteralNode::gen()
{
    auto str = compiler.get_builder().CreateGlobalString(value.data());
    return str;
}

llvm::Type* StringLiteralNode::get_type(llvm::LLVMContext& context) const
{
    return llvm::PointerType::get(llvm::IntegerType::get(context, 8), 0);
}

llvm::Value* StringLiteralNode::trivial_gen()
{
    return gen();
}

bool StringLiteralNode::is_trivially_evaluable() const
{
    return true;
}
