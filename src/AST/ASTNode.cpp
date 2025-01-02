#include <cassert>
#include <fmt/core.h>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "kyoto/SymbolTable.h"
#include "kyoto/TypeResolver.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Casting.h"

namespace llvm {
class LLVMContext;
class Value;
}

llvm::Type* ASTNode::get_llvm_type(const KType* type, llvm::LLVMContext& context)
{
    auto primitive_type = dynamic_cast<const PrimitiveType*>(type);
    assert(primitive_type && "Only primitive types are supported");

    switch (primitive_type->get_kind()) {
    case PrimitiveType::Kind::Boolean:
        return llvm::Type::getInt1Ty(context);
    case PrimitiveType::Kind::Char:
        return llvm::Type::getInt8Ty(context);
    case PrimitiveType::Kind::I8:
        return llvm::Type::getInt8Ty(context);
    case PrimitiveType::Kind::I16:
        return llvm::Type::getInt16Ty(context);
    case PrimitiveType::Kind::I32:
        return llvm::Type::getInt32Ty(context);
    case PrimitiveType::Kind::I64:
        return llvm::Type::getInt64Ty(context);
    case PrimitiveType::Kind::F32:
        return llvm::Type::getFloatTy(context);
    case PrimitiveType::Kind::F64:
        return llvm::Type::getDoubleTy(context);
    case PrimitiveType::Kind::Void:
        return llvm::Type::getVoidTy(context);
    case PrimitiveType::Kind::Unknown:
        assert(false && "Unknown type");
    }
    return nullptr;
}

ProgramNode::ProgramNode(std::vector<ASTNode*> nodes, ModuleCompiler& compiler)
    : nodes(std::move(nodes))
    , compiler(compiler)
{
}

std::string ProgramNode::to_string() const
{
    std::string str;
    for (const auto* node : nodes) {
        str += node->to_string() + "\n";
    }
    return fmt::format("ProgramNode([{}])", str);
}

llvm::Value* ProgramNode::gen()
{
    compiler.push_scope();

    for (auto* node : nodes)
        node->gen();

    assert(compiler.n_scopes() == 1 && "Unbalanced scopes");
    compiler.pop_scope();
    return nullptr;
}

IdentifierExpressionNode::IdentifierExpressionNode(std::string name, ModuleCompiler& compiler)
    : name(name)
    , compiler(compiler)
{
}

std::string IdentifierExpressionNode::to_string() const
{
    return fmt::format("IdentifierNode({})", name);
}

llvm::Value* IdentifierExpressionNode::gen()
{
    auto symbol_opt = compiler.get_symbol(name);
    if (!symbol_opt.has_value()) {
        assert(false && "Unknown symbol");
    }
    auto symbol = symbol_opt.value();
    return compiler.get_builder().CreateLoad(symbol.alloc->getAllocatedType(), symbol.alloc, name);
}

llvm::Type* IdentifierExpressionNode::get_type(llvm::LLVMContext& context) const
{
    auto symbol = compiler.get_symbol(name);
    if (!symbol.has_value()) {
        assert(false && "Unknown symbol");
    }
    auto stored_type = symbol.value().kind;
    return symbol.value().alloc->getAllocatedType();
}

DeclarationStatementNode::DeclarationStatementNode(std::string name, KType* ktype, ModuleCompiler& compiler)
    : name(name)
    , type(ktype)
    , compiler(compiler)
{
}

std::string DeclarationStatementNode::to_string() const
{
    return fmt::format("DeclarationNode({}, {})", name, type->to_string());
}

llvm::Value* DeclarationStatementNode::gen()
{
    auto* ltype = get_llvm_type(type, compiler.get_context());
    auto* val = new llvm::AllocaInst(ltype, 0, name, compiler.get_builder().GetInsertBlock());
    compiler.add_symbol(name, Symbol::primitive(val, dynamic_cast<PrimitiveType*>(type)->get_kind()));
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

std::string FullDeclarationStatementNode::to_string() const
{
    return fmt::format("FullDeclarationNode({}, {}, {})", name, type->to_string(), expr->to_string());
}

llvm::Value* ExpressionNode::promoted_trivially_gen(ExpressionNode* expr, ModuleCompiler& compiler, KType* target_ktype,
                                                    const std::string& target_name)
{
    if (!expr->is_trivially_evaluable()) return nullptr;

    auto* target_type = dynamic_cast<PrimitiveType*>(target_ktype);
    auto* ltype = get_llvm_type(target_type, compiler.get_context());
    auto expr_ktype = PrimitiveType::from_llvm_type(expr->get_type(compiler.get_context()));

    auto trivial_val = expr->trivial_gen();
    auto* constant_int = llvm::dyn_cast<llvm::ConstantInt>(trivial_val);
    assert(constant_int && "Trivial value must be a constant int");
    auto int_val = target_type->is_boolean() || expr_ktype.is_boolean() ? constant_int->getZExtValue()
                                                                        : constant_int->getSExtValue();
    if (!compiler.get_type_resolver().fits_in(int_val, target_type->get_kind())) {
        throw std::runtime_error(fmt::format("Value of RHS `{}` = `{}` does not fit in type `{}`{}", expr->to_string(),
                                             int_val, target_type->to_string(),
                                             target_name.empty() ? "" : fmt::format(" for `{}`", target_name)));
    }
    return llvm::ConstantInt::get(ltype, int_val, true);
}

llvm::Value* ExpressionNode::handle_integer_conversion(ExpressionNode* expr, KType* target_ktype,
                                                       ModuleCompiler& compiler, const std::string& what,
                                                       const std::string& target_name)
{
    auto* target_type = dynamic_cast<PrimitiveType*>(target_ktype);
    auto* ltype = get_llvm_type(target_type, compiler.get_context());
    auto expr_ktype = PrimitiveType::from_llvm_type(expr->get_type(compiler.get_context()));
    auto* expr_val = expr->gen();
    bool is_compatible = compiler.get_type_resolver().promotable_to(expr_ktype.get_kind(), target_type->get_kind());
    bool is_trivially_evaluable = expr->is_trivially_evaluable();

    if (!is_compatible && !is_trivially_evaluable) {
        throw std::runtime_error(fmt::format("Cannot {} value of type `{}`. Expected `{}`{}", what,
                                             expr_ktype.to_string(), target_type->to_string(),
                                             target_name.empty() ? "" : fmt::format(" for `{}`", target_name)));
    }

    if (auto* trivial = ExpressionNode::promoted_trivially_gen(expr, compiler, target_ktype, target_name); trivial) {
        return trivial;
    } else {
        if (expr_ktype.width() > target_type->width()) {
            return compiler.get_builder().CreateTrunc(expr_val, ltype);
        } else if (expr_ktype.width() < target_type->width()) {
            return compiler.get_builder().CreateSExt(expr_val, ltype);
        }
    }

    return expr_val;
}

llvm::Value* FullDeclarationStatementNode::gen()
{
    auto* ltype = get_llvm_type(type, compiler.get_context());
    auto* alloca = new llvm::AllocaInst(ltype, 0, name, compiler.get_builder().GetInsertBlock());
    auto* expr_val = ExpressionNode::handle_integer_conversion(expr, type, compiler, "assign", name);

    compiler.get_builder().CreateStore(expr_val, alloca);
    compiler.add_symbol(name, Symbol::primitive(alloca, dynamic_cast<PrimitiveType*>(type)->get_kind()));

    return alloca;
}

ReturnStatementNode::ReturnStatementNode(ExpressionNode* expr, ModuleCompiler& compiler)
    : expr(expr)
    , compiler(compiler)
{
}

std::string ReturnStatementNode::to_string() const
{
    return fmt::format("ReturnNode({})", expr->to_string());
}

llvm::Value* ReturnStatementNode::gen()
{
    auto* fn_ret_type = compiler.get_fn_return_type();
    auto* expr_val = ExpressionNode::handle_integer_conversion(expr, fn_ret_type, compiler, "return");
    return compiler.get_builder().CreateRet(expr_val);
}

UnaryNode::UnaryNode(ExpressionNode* expr, std::string op, ModuleCompiler& compiler)
    : expr(expr)
    , op(op)
    , compiler(compiler)
{
}

std::string UnaryNode::to_string() const
{
    return fmt::format("UnaryNode({}, {})", op, expr->to_string());
}

llvm::Value* UnaryNode::gen()
{
    auto* expr_val = expr->gen();
    auto expr_ltype = expr->get_type(compiler.get_context());
    auto expr_ktype = PrimitiveType::from_llvm_type(expr_ltype);

    if (op == "-") return compiler.get_builder().CreateNeg(expr_val, "negval");
    else if (op == "+") return expr_val;

    return nullptr;
}

llvm::Type* UnaryNode::get_type(llvm::LLVMContext& context) const
{
    return expr->get_type(context);
}

BlockNode::BlockNode(std::vector<ASTNode*> nodes, ModuleCompiler& compiler)
    : nodes(std::move(nodes))
    , compiler(compiler)
{
}

std::string BlockNode::to_string() const
{
    std::string str;
    for (const auto* node : nodes) {
        str += node->to_string() + ", ";
    }
    return fmt::format("BlockNode([{}])", str);
}

llvm::Value* BlockNode::gen()
{
    compiler.push_scope();

    for (auto* node : nodes) {
        node->gen();
    }

    compiler.pop_scope();
    return nullptr;
}

FunctionNode::FunctionNode(const std::string& name, std::vector<Parameter> args, KType* ret_type, ASTNode* body,
                           ModuleCompiler& compiler)
    : name(name)
    , args(std::move(args))
    , ret_type(ret_type)
    , body(body)
    , compiler(compiler)
{
}

std::string FunctionNode::to_string() const
{
    std::string args_str;
    for (const auto& arg : args) {
        args_str += arg.name + ": " + arg.type->to_string() + ", ";
    }
    return fmt::format("FunctionNode({}, [{}], [{}])", name, args_str, body->to_string());
}

llvm::Value* FunctionNode::gen()
{
    auto* return_ltype = get_llvm_type(ret_type, compiler.get_context());
    auto* func_type = llvm::FunctionType::get(return_ltype, get_arg_types(), false);
    auto* func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, name, compiler.get_module());
    auto* entry = llvm::BasicBlock::Create(compiler.get_context(), "func_entry", func);
    compiler.get_builder().SetInsertPoint(entry);

    compiler.push_fn_return_type(ret_type);
    body->gen();
    compiler.pop_fn_return_type();

    return func;
}

std::vector<llvm::Type*> FunctionNode::get_arg_types() const
{
    std::vector<llvm::Type*> types;
    for (const auto& arg : args) {
        auto* ltype = get_llvm_type(arg.type, compiler.get_context());
        types.push_back(ltype);
    }
    return types;
}