#include "kyoto/AST/ReturnStatement.h"

#include <assert.h>
#include <fmt/core.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <stdexcept>
#include <vector>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/Expressions/ExpressionNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"

namespace llvm {
class Value;
}

ReturnStatementNode::ReturnStatementNode(ExpressionNode* expr, ModuleCompiler& compiler)
    : expr(expr)
    , compiler(compiler)
{
}

ReturnStatementNode::~ReturnStatementNode()
{
    delete expr;
}

std::string ReturnStatementNode::to_string() const
{
    return fmt::format("ReturnNode({})", expr->to_string());
}

llvm::Value* ReturnStatementNode::gen()
{
    auto* fn_ret_type = compiler.get_fn_return_type();

    if (fn_ret_type->is_void()) {
        validate_void_return();
        return compiler.get_builder().CreateRetVoid();
    }

    if (!expr) {
        throw std::runtime_error(fmt::format("Expected return type `{}`", fn_ret_type->to_string()));
    }

    llvm::Value* expr_val = generate_return_value();
    return compiler.get_builder().CreateRet(expr_val);
}

void ReturnStatementNode::validate_void_return() const
{
    if (expr) {
        throw std::runtime_error(fmt::format("Expected return type void, got expression `{}` (type `{}`)",
                                             expr->to_string(), expr->get_ktype()->to_string()));
    }
}

llvm::Value* ReturnStatementNode::generate_return_value() const
{
    auto* fn_ret_type = compiler.get_fn_return_type();
    auto fn_name = compiler.get_current_function_node()->get_name();

    if (are_compatible_integers_or_booleans()) {
        return ExpressionNode::handle_integer_conversion(expr, fn_ret_type, compiler, "return", fn_name);
    }

    if (fn_ret_type->is_string() && expr->get_ktype()->is_string()) {
        return expr->gen();
    }

    if (are_compatible_pointer_types()) {
        return generate_pointer_return_value();
    }

    throw std::runtime_error(
        fmt::format("Type of expression `{}` (type: `{}`) can't be returned from the function `{}`. Expected type `{}`",
                    expr->to_string(), expr->get_ktype()->to_string(), fn_name, fn_ret_type->to_string()));
}

bool ReturnStatementNode::are_compatible_integers_or_booleans() const
{
    auto* fn_ret_type = compiler.get_fn_return_type();
    return (fn_ret_type->is_integer() && expr->get_ktype()->is_integer())
        || (fn_ret_type->is_boolean() && expr->get_ktype()->is_boolean());
}

bool ReturnStatementNode::are_compatible_pointer_types() const
{
    auto* fn_ret_type = compiler.get_fn_return_type();
    auto* expr_ktype = expr->get_ktype();
    return fn_ret_type->is_pointer() && expr_ktype->is_pointer() && fn_ret_type->operator==(*expr_ktype);
}

llvm::Value* ReturnStatementNode::generate_pointer_return_value() const
{
    llvm::Value* expr_val = expr->gen_ptr();
    assert(expr_val && "Expression must be a pointer");
    auto fn_ret_type = compiler.get_fn_return_type();
    return compiler.get_builder().CreateLoad(get_llvm_type(fn_ret_type, compiler), expr_val);
}

std::vector<ASTNode*> ReturnStatementNode::get_children() const
{
    return { expr };
}