#include "kyoto/AST/DeclarationNodes.h"

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

    if (type->is_string()) {
        compiler.add_symbol(name, Symbol { val, false, type });
    } else {
        compiler.add_symbol(name, Symbol::primitive(val, dynamic_cast<PrimitiveType*>(type)->get_kind()));
    }

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
    delete type;
    delete expr;
}

std::string FullDeclarationStatementNode::to_string() const
{
    return fmt::format("FullDeclarationNode({}, {}, {})", name, type->to_string(), expr->to_string());
}

llvm::Value* FullDeclarationStatementNode::gen()
{
    if (!type) {
        auto* expr_type = expr->get_type(compiler.get_context());
        type = KType::from_llvm_type(expr_type);
    }

    auto* pt = KType::from_llvm_type(expr->get_type(compiler.get_context()));

    if (type->is_void()) {
        throw std::runtime_error(fmt::format("Cannot declare variable `{}` of type `void`", name));
    } else if (pt->is_void()) {
        throw std::runtime_error(fmt::format("Cannot assign value of type `void` to variable `{}`", name));
    }

    auto* ltype = get_llvm_type(type, compiler.get_context());
    auto* alloca = new llvm::AllocaInst(ltype, 0, name, compiler.get_builder().GetInsertBlock());

    llvm::Value* expr_val = nullptr;
    if (type->is_integer() && pt->is_integer() || type->is_boolean() && pt->is_boolean()) {
        expr_val = ExpressionNode::handle_integer_conversion(expr, type, compiler, "assign", name);
    } else if (type->is_string() && pt->is_string()) {
        expr_val = expr->gen();
    } else {
        throw std::runtime_error(
            fmt::format("Type of expression `{}` (type: `{}`) can't be assigned to the variable `{}` (type `{}`)",
                        expr->to_string(), pt->to_string(), name, type->to_string()));
    }

    compiler.get_builder().CreateStore(expr_val, alloca);
    if (type->is_string()) {
        compiler.add_symbol(name, Symbol { alloca, false, pt });
    } else {
        compiler.add_symbol(name, Symbol::primitive(alloca, dynamic_cast<PrimitiveType*>(type)->get_kind()));
    }

    return alloca;
}