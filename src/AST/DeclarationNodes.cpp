#include "kyoto/AST/DeclarationNodes.h"

#include <assert.h>
#include <fmt/core.h>
#include <stdexcept>

#include "kyoto/AST/ExpressionNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "kyoto/SymbolTable.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

DeclarationStatementNode::DeclarationStatementNode(std::string name, KType* ktype, ModuleCompiler& compiler)
    : name(name)
    , type(ktype)
    , compiler(compiler)
{
}

DeclarationStatementNode::~DeclarationStatementNode()
{
    delete type;
}

std::string DeclarationStatementNode::to_string() const
{
    return fmt::format("DeclarationNode({}, {})", name, type->to_string());
}

llvm::Value* DeclarationStatementNode::gen()
{
    auto* ltype = get_llvm_type(type, compiler.get_context());
    auto* val = new llvm::AllocaInst(ltype, 0, name, compiler.get_builder().GetInsertBlock());

    compiler.add_symbol(name, Symbol { val, !type->is_pointer(), type });
    return val;
}

FullDeclarationStatementNode::FullDeclarationStatementNode(std::string name, KType* ktype, ExpressionNode* expr,
                                                           ModuleCompiler& compiler)
    : name(name)
    , type(ktype)
    , expr(expr)
    , compiler(compiler)
{
}

FullDeclarationStatementNode::~FullDeclarationStatementNode()
{
    if (type && !type->is_void()) delete type;
    delete expr;
}

std::string FullDeclarationStatementNode::to_string() const
{
    return fmt::format("FullDeclarationNode({}, {}, {})", name, type->to_string(), expr->to_string());
}

llvm::Value* FullDeclarationStatementNode::gen()
{
    if (!type) {
        type = expr->get_ktype()->copy();
    }

    const auto expr_ktype = expr->get_ktype();
    if (type->is_void()) {
        throw std::runtime_error(fmt::format("Cannot declare variable `{}` of type `void`", name));
    }

    if (expr_ktype->is_void()) {
        throw std::runtime_error(fmt::format("Cannot assign value of type `void` to variable `{}`", name));
    }

    auto* ltype = get_llvm_type(type, compiler.get_context());
    auto* alloca = new llvm::AllocaInst(ltype, 0, name, compiler.get_builder().GetInsertBlock());

    llvm::Value* expr_val = nullptr;
    if ((type->is_integer() && expr_ktype->is_integer()) || (type->is_boolean() && expr_ktype->is_boolean())) {
        expr_val = ExpressionNode::handle_integer_conversion(expr, type, compiler, "assign", name);
    } else if (type->is_string() && expr_ktype->is_string()) {
        expr_val = expr->gen();
    } else {
        if (type->is_pointer() && expr_ktype->is_pointer() && type->operator==(*expr_ktype)) {
            expr_val = expr->gen_ptr();
            assert(expr_val && "Expression must be a pointer");
        } else {
            throw std::runtime_error(
                fmt::format("Type of expression `{}` (type: `{}`) can't be assigned to the variable `{}` (type `{}`)",
                            expr->to_string(), expr_ktype->to_string(), name, type->to_string()));
        }
    }

    compiler.get_builder().CreateStore(expr_val, alloca);
    compiler.add_symbol(name, Symbol { alloca, !type->is_pointer(), type });
    return alloca;
}
