#include "kyoto/AST/StringLiteralNode.h"

#include <fmt/core.h>
#include <regex>
#include <utility>

#include "kyoto/ModuleCompiler.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"

StringLiteralNode::StringLiteralNode(std::string value, ModuleCompiler& compiler)
    : type(nullptr)
    , value(std::move(value))
    , compiler(compiler)
{
}

StringLiteralNode::~StringLiteralNode()
{
    delete type;
}

std::string StringLiteralNode::to_string() const
{
    auto escaped_value = std::regex_replace(value, std::regex("\n"), "\\n");
    return fmt::format("StringLiteralNode(\"{}\")", escaped_value);
}

llvm::Value* StringLiteralNode::gen()
{
    auto str = compiler.get_builder().CreateGlobalString(value.data());
    return str;
}

llvm::Type* StringLiteralNode::gen_type() const
{
    return llvm::PointerType::get(llvm::IntegerType::get(compiler.get_context(), 8), 0);
}

llvm::Value* StringLiteralNode::trivial_gen()
{
    return gen();
}

bool StringLiteralNode::is_trivially_evaluable() const
{
    return true;
}

KType* StringLiteralNode::get_ktype() const
{
    if (!type) {
        type = new PointerType(new PrimitiveType(PrimitiveType::Kind::Char));
    }
    return type;
}