#include "kyoto/AST/DeclarationNodes.h"

#include <assert.h>
#include <fmt/core.h>
#include <stdexcept>

#include "kyoto/AST/Expressions/ExpressionNode.h"
#include "kyoto/AST/Expressions/FunctionCallNode.h"
#include "kyoto/AST/Expressions/IdentifierNode.h"
#include "kyoto/AST/Expressions/UnaryNode.h"
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
    auto* ltype = get_llvm_type(type, compiler);
    auto* val = new llvm::AllocaInst(ltype, 0, name, compiler.get_builder().GetInsertBlock());

    compiler.add_symbol(name, Symbol { val, type });
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
    initialize_type();
    validate_type_not_void();
    validate_expression_not_void();

    auto* alloca = create_alloca();
    llvm::Value* expr_val = generate_expression_value(alloca);

    store_val_and_register_symbol(expr_val, alloca);
    return alloca;
}

void FullDeclarationStatementNode::initialize_type()
{
    if (!type) {
        type = expr->get_ktype()->copy();
    }
}

void FullDeclarationStatementNode::validate_type_not_void() const
{
    if (type->is_void()) {
        throw std::runtime_error(fmt::format("Cannot declare variable `{}` of type `void`", name));
    }
}

void FullDeclarationStatementNode::validate_expression_not_void() const
{
    const auto* expr_ktype = expr->get_ktype();
    bool constructor_call = expr->is<FunctionCall>() && expr->as<FunctionCall>()->is_constructor_call();
    if (expr_ktype->is_void() && !constructor_call)
        throw std::runtime_error(fmt::format("Cannot assign value of type `void` to variable `{}`", name));
}

llvm::AllocaInst* FullDeclarationStatementNode::create_alloca() const
{
    auto* ltype = get_llvm_type(type, compiler);
    return new llvm::AllocaInst(ltype, 0, name, compiler.get_builder().GetInsertBlock());
}

llvm::Value* FullDeclarationStatementNode::generate_expression_value(llvm::AllocaInst* alloca)
{
    const auto expr_ktype = expr->get_ktype();

    if ((type->is_integer() && expr_ktype->is_integer()) || (type->is_boolean() && expr_ktype->is_boolean())) {
        return ExpressionNode::handle_integer_conversion(expr, type, compiler, "assign", name);
    }

    if (type->is_string() && expr_ktype->is_string()) {
        return expr->gen();
    }

    if (is_assigning_to_class_instance()) {
        return handle_constructor_call(alloca);
    }

    if (type->is_pointer() && expr_ktype->is_pointer() && type->operator==(*expr_ktype)) {
        auto* expr_val = expr->gen();
        assert(expr_val && "Expression must be a pointer");
        return expr_val;
    }

    if (type->is_array() && expr_ktype->is_array() && type->operator==(*expr_ktype)) {
        return expr->gen();
    }

    std::string expr_ktype_str = expr_ktype->to_string();
    if (expr_ktype->is_void() && expr->is<FunctionCall>() && expr->as<FunctionCall>()->is_constructor_call()) {
        expr_ktype_str = fmt::format("class {}", expr->to_string().substr(0, expr->to_string().find('_')));
    }

    throw std::runtime_error(
        fmt::format("Type of expression `{}` (type: `{}`) can't be assigned to the variable `{}` (type `{}`)",
                    expr->to_string(), expr_ktype_str, name, type->to_string()));
}

bool FullDeclarationStatementNode::is_assigning_to_class_instance() const
{
    return type->is_class() && expr->is<FunctionCall>() && expr->as<FunctionCall>()->is_constructor_call();
}

llvm::Value* FullDeclarationStatementNode::handle_constructor_call(llvm::AllocaInst* alloca) const
{
    ExpressionNode* self = new IdentifierExpressionNode(name, compiler);
    self = new UnaryNode(self, UnaryNode::UnaryOp::AddressOf, compiler);
    compiler.add_symbol(name, Symbol { alloca, type });
    auto* f = expr->as<FunctionCall>();
    f->insert_arg(self, 0);
    auto _ = f->gen();
    return alloca;
}

void FullDeclarationStatementNode::store_val_and_register_symbol(llvm::Value* expr_val, llvm::AllocaInst* alloca) const
{
    if (is_assigning_to_class_instance()) return;
    compiler.get_builder().CreateStore(expr_val, alloca);
    compiler.add_symbol(name, Symbol { alloca, type });
}

std::vector<ASTNode*> FullDeclarationStatementNode::get_children() const
{
    return { expr };
}