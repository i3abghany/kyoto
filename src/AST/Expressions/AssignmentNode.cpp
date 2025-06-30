#include "kyoto/AST/Expressions/AssignmentNode.h"

#include <assert.h>
#include <format>
#include <stdexcept>

#include "kyoto/AST/Expressions/ExpressionNode.h"
#include "kyoto/AST/Expressions/UnaryNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
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
    return std::format("Assignment({}, {})", assignee->to_string(), expr->to_string());
}

llvm::Value* AssignmentNode::gen_deref_assignment() const
{
    auto* deref = dynamic_cast<UnaryNode*>(assignee);
    assert(deref && "Expected dereference unary node");

    auto* pktype = deref->get_ktype();
    const auto* expr_ktype = expr->get_ktype();

    if (!expr_ktype->operator==(*pktype)) {
        throw std::runtime_error(std::format("Cannot assign expression {} (type {}) to lvalue {} (type {})",
                                             expr->to_string(), expr_ktype->to_string(), assignee->to_string(),
                                             pktype->to_string()));
    }

    auto* addr = assignee->gen_ptr();
    auto* expr_val = expr->gen();
    return compiler.get_builder().CreateStore(expr_val, addr);
}

llvm::Value* AssignmentNode::gen()
{
    validate_lvalue();

    if (assignee->is<UnaryNode>()) {
        return gen_deref_assignment();
    }

    auto* alloc = assignee->gen_ptr();
    auto* type = assignee->get_ktype();
    auto name = assignee->to_string();

    llvm::Value* expr_val = generate_expression_value(type, name);
    compiler.get_builder().CreateStore(expr_val, alloc);

    return expr_val;
}

void AssignmentNode::validate_lvalue() const
{
    if (assignee->is_trivially_evaluable()) {
        throw std::runtime_error(std::format("Cannot assign to a non-lvalue `{}`", assignee->to_string()));
    }
}

llvm::Value* AssignmentNode::generate_expression_value(const KType* type, const std::string& name) const
{

    if (are_compatible_integers_or_booleans(type)) {
        return handle_integer_conversion(expr, type, compiler, "assign", name);
    }

    if (type->is_string() && expr->get_ktype()->is_string()) {
        return expr->gen();
    }

    if (are_compatible_pointer_types(type)) {
        return expr->gen();
    }

    throw std::runtime_error(
        std::format("Type of expression `{}` (type: `{}`) can't be assigned to the variable `{}` (type `{}`)",
                    expr->to_string(), expr->get_ktype()->to_string(), name, type->to_string()));
}

bool AssignmentNode::are_compatible_integers_or_booleans(const KType* type) const
{
    return (type->is_integer() && expr->get_ktype()->is_integer())
        || (type->is_boolean() && expr->get_ktype()->is_boolean());
}

bool AssignmentNode::are_compatible_pointer_types(const KType* type) const
{
    return type->is_pointer() && expr->get_ktype()->is_pointer() && type->operator==(*expr->get_ktype());
}

llvm::Type* AssignmentNode::gen_type() const
{
    return expr->gen_type();
}

llvm::Value* AssignmentNode::trivial_gen()
{
    auto* alloc = assignee->gen_ptr();
    auto* type = assignee->get_ktype();
    auto name = assignee->to_string();

    auto* expr_val = handle_integer_conversion(expr, type, compiler, "assign", name);
    compiler.get_builder().CreateStore(expr_val, alloc);

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
