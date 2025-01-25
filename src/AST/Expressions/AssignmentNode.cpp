#include "kyoto/AST/Expressions/AssignmentNode.h"

#include <assert.h>
#include <fmt/core.h>
#include <optional>
#include <stdexcept>

#include "kyoto/AST/Expressions/ExpressionNode.h"
#include "kyoto/AST/Expressions/IdentifierNode.h"
#include "kyoto/AST/Expressions/UnaryNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "kyoto/SymbolTable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"

namespace llvm {
class Value;
}

AssignmentNode::AssignmentNode(ExpressionNode* assignee, ExpressionNode* expr, ModuleCompiler& compiler)
    : assignee(assignee)
    , expr(expr)
    , compiler(compiler)
{
}

AssignmentNode::~AssignmentNode()
{
    delete assignee;
    delete expr;
}

std::string AssignmentNode::to_string() const
{
    return fmt::format("Assignment({}, {})", assignee->to_string(), expr->to_string());
}

llvm::Value* AssignmentNode::gen_deref_assignment() const
{
    auto* deref = dynamic_cast<UnaryNode*>(assignee);
    assert(deref && "Expected dereference unary node");

    auto* pktype = deref->get_ktype();
    const auto* expr_ktype = expr->get_ktype();

    if (!expr_ktype->operator==(*pktype)) {
        throw std::runtime_error(fmt::format("Cannot assign expression {} (type {}) to lvalue {} (type {})",
                                             expr->to_string(), expr_ktype->to_string(), assignee->to_string(),
                                             pktype->to_string()));
    }

    auto* addr = assignee->gen_ptr();
    auto* expr_val = expr->gen();
    return compiler.get_builder().CreateStore(expr_val, addr);
}

llvm::Value* AssignmentNode::gen()
{
    if (assignee->is_trivially_evaluable()) {
        throw std::runtime_error(fmt::format("Cannot assign to a non-lvalue `{}`", assignee->to_string()));
    }

    const auto* identifier = dynamic_cast<IdentifierExpressionNode*>(assignee);

    if (dynamic_cast<UnaryNode*>(assignee)) return gen_deref_assignment();

    std::string name;
    if (identifier) name = identifier->get_name();

    auto symbol_opt = compiler.get_symbol(name);
    if (!symbol_opt.has_value()) {
        throw std::runtime_error(fmt::format("Unknown symbol `{}`", name));
    }

    auto symbol = symbol_opt.value();
    auto type = symbol.type;

    llvm::Value* expr_val = nullptr;
    auto* expr_ktype = expr->get_ktype();
    if ((type->is_integer() && expr_ktype->is_integer()) || (type->is_boolean() && expr_ktype->is_boolean())) {
        expr_val = handle_integer_conversion(expr, type, compiler, "assign", name);
    } else if (type->is_string() && expr_ktype->is_string()) {
        expr_val = expr->gen();
    } else {
        if (type->is_pointer() && expr_ktype->is_pointer() && type->operator==(*expr_ktype)) {
            expr_val = expr->gen_ptr();
        } else {
            throw std::runtime_error(
                fmt::format("Type of expression `{}` (type: `{}`) can't be assigned to the variable `{}` (type `{}`)",
                            expr->to_string(), expr_ktype->to_string(), name, type->to_string()));
        }
    }

    compiler.get_builder().CreateStore(expr_val, symbol.alloc);
    return expr_val;
}

llvm::Type* AssignmentNode::gen_type() const
{
    return expr->gen_type();
}

llvm::Value* AssignmentNode::trivial_gen()
{
    auto* identifier = dynamic_cast<IdentifierExpressionNode*>(assignee);
    if (!identifier) {
        throw std::runtime_error(
            fmt::format("Expected lvalue on the left side of assignment, got `{}`", assignee->to_string()));
    }

    const auto& name = identifier->get_name();

    auto symbol_opt = compiler.get_symbol(name);
    if (!symbol_opt.has_value()) {
        assert(false && "Unknown symbol");
    }

    const auto symbol = symbol_opt.value();
    auto* type = symbol.type;
    auto* expr_val = handle_integer_conversion(expr, type, compiler, "assign", name);
    compiler.get_builder().CreateStore(expr_val, symbol.alloc);

    return expr_val;
}

KType* AssignmentNode::get_ktype() const
{
    return expr->get_ktype();
}

bool AssignmentNode::is_trivially_evaluable() const
{
    return expr->is_trivially_evaluable();
}
