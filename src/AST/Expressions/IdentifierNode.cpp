#include "kyoto/AST/Expressions/IdentifierNode.h"

#include <format>
#include <optional>
#include <stdexcept>
#include <utility>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/ModuleCompiler.h"
#include "kyoto/SymbolTable.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

namespace {

std::optional<FunctionNode*> resolve_function_metadata(const std::string& name, ModuleCompiler& compiler)
{
    if (auto fn = compiler.get_function(name); fn.has_value()) return fn.value();
    if (name != "main") {
        if (auto fn = compiler.get_function(compiler.qualify_local_name(name)); fn.has_value()) return fn.value();
    }
    return std::nullopt;
}

std::string resolve_llvm_function_name(FunctionNode* fn)
{
    if (fn->get_linkage_name() == "main" && !fn->is_external()) return "main";
    if (fn->is_external()) return fn->get_linkage_name();
    return fn->get_linkage_name() + "_" + std::to_string(fn->get_params().size());
}

FunctionType* copy_function_type(FunctionNode* fn)
{
    std::vector<KType*> param_types;
    param_types.reserve(fn->get_params().size());
    for (const auto& param : fn->get_params())
        param_types.push_back(param.type->copy());
    return new FunctionType(std::move(param_types), fn->get_ret_type()->copy());
}

} // namespace

IdentifierExpressionNode::IdentifierExpressionNode(std::string name, ModuleCompiler& compiler)
    : name(std::move(name))
    , compiler(compiler)
{
}

std::string IdentifierExpressionNode::to_string() const
{
    return std::format("Identifier({})", name);
}

llvm::Value* IdentifierExpressionNode::gen()
{
    auto symbol_opt = compiler.get_symbol(name);
    if (symbol_opt.has_value()) {
        auto symbol = symbol_opt.value();
        return compiler.get_builder().CreateLoad(symbol.alloc->getAllocatedType(), symbol.alloc, name);
    }

    auto fn = resolve_function_metadata(name, compiler);
    if (!fn.has_value()) throw std::runtime_error(std::format("Unknown symbol `{}`", name));

    auto* llvm_fn = compiler.get_module()->getFunction(resolve_llvm_function_name(fn.value()));
    if (!llvm_fn) throw std::runtime_error(std::format("Unknown function `{}`", name));
    return llvm_fn;
}

llvm::Value* IdentifierExpressionNode::gen_ptr() const
{
    auto symbol_opt = compiler.get_symbol(name);
    if (!symbol_opt.has_value()) {
        throw std::runtime_error(std::format("Unknown symbol `{}`", name));
    }
    auto symbol = symbol_opt.value();
    return symbol.alloc;
}

llvm::Type* IdentifierExpressionNode::gen_type() const
{
    auto symbol = compiler.get_symbol(name);
    if (symbol.has_value()) {
        return symbol.value().alloc->getAllocatedType();
    }

    auto fn = resolve_function_metadata(name, compiler);
    if (!fn.has_value()) throw std::runtime_error(std::format("Unknown symbol `{}`", name));
    if (!resolved_function_type) resolved_function_type.reset(copy_function_type(fn.value()));
    return ASTNode::get_llvm_type(resolved_function_type.get(), compiler);
}

KType* IdentifierExpressionNode::get_ktype() const
{
    auto symbol = compiler.get_symbol(name);
    if (symbol.has_value()) {
        return symbol.value().type;
    }

    auto fn = resolve_function_metadata(name, compiler);
    if (!fn.has_value()) throw std::runtime_error(std::format("Unknown symbol `{}`", name));
    if (!resolved_function_type) resolved_function_type.reset(copy_function_type(fn.value()));
    return resolved_function_type.get();
}
