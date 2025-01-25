#include "kyoto/AST/ReturnStatement.h"

#include <assert.h>
#include <fmt/core.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <stdexcept>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/Expressions/ExpressionNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"

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

    const auto* expr_ktype = expr ? expr->get_ktype() : KType::get_void();
    auto fn_name = compiler.get_current_function_node()->get_name();

    if (fn_ret_type->is_void()) {
        if (expr)
            throw std::runtime_error(fmt::format("Expected return type void, got expression `{}` (type `{}`)",
                                                 expr->to_string(), expr->get_ktype()->to_string()));
        return compiler.get_builder().CreateRetVoid();
    }

    if (!expr) throw std::runtime_error(fmt::format("Expected return type `{}`", fn_ret_type->to_string()));

    llvm::Value* expr_val = nullptr;
    if ((fn_ret_type->is_integer() && expr_ktype->is_integer())
        || (fn_ret_type->is_boolean() && expr_ktype->is_boolean())) {
        expr_val = ExpressionNode::handle_integer_conversion(expr, fn_ret_type, compiler, "return", fn_name);
    } else if (fn_ret_type->is_string() && expr_ktype->is_string()) {
        expr_val = expr->gen();
    } else {
        if (fn_ret_type->is_pointer() && expr_ktype->is_pointer() && fn_ret_type->operator==(*expr_ktype)) {
            expr_val = expr->gen_ptr();
            expr_val = compiler.get_builder().CreateLoad(get_llvm_type(fn_ret_type, compiler), expr_val);
            assert(expr_val && "Expression must be a pointer");
        } else {
            throw std::runtime_error(fmt::format(
                "Type of expression `{}` (type: `{}`) can't be returned from the function `{}`. Expected type `{}`",
                expr->to_string(), expr_ktype->to_string(), fn_name, fn_ret_type->to_string()));
        }
    }

    return compiler.get_builder().CreateRet(expr_val);
}