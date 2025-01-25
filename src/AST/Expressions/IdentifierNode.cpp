#include "kyoto/AST/Expressions/IdentifierNode.h"

#include <fmt/core.h>
#include <optional>
#include <stdexcept>
#include <utility>

#include "kyoto/ModuleCompiler.h"
#include "kyoto/SymbolTable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"

IdentifierExpressionNode::IdentifierExpressionNode(std::string name, ModuleCompiler& compiler)
    : name(std::move(name))
    , compiler(compiler)
{
}

std::string IdentifierExpressionNode::to_string() const
{
    return fmt::format("Identifier({})", name);
}

llvm::Value* IdentifierExpressionNode::gen()
{
    auto symbol_opt = compiler.get_symbol(name);
    if (!symbol_opt.has_value()) {
        throw std::runtime_error(fmt::format("Unknown symbol `{}`", name));
    }
    auto symbol = symbol_opt.value();
    return compiler.get_builder().CreateLoad(symbol.alloc->getAllocatedType(), symbol.alloc, name);
}

llvm::Value* IdentifierExpressionNode::gen_ptr() const
{
    auto symbol_opt = compiler.get_symbol(name);
    if (!symbol_opt.has_value()) {
        throw std::runtime_error(fmt::format("Unknown symbol `{}`", name));
    }
    auto symbol = symbol_opt.value();
    return symbol.alloc;
}

llvm::Type* IdentifierExpressionNode::gen_type() const
{
    auto symbol = compiler.get_symbol(name);
    if (!symbol.has_value()) {
        throw std::runtime_error(fmt::format("Unknown symbol `{}`", name));
    }
    return symbol.value().alloc->getAllocatedType();
}

KType* IdentifierExpressionNode::get_ktype() const
{
    auto symbol = compiler.get_symbol(name);
    if (!symbol.has_value()) {
        throw std::runtime_error(fmt::format("Unknown symbol `{}`", name));
    }
    return symbol.value().type;
}
