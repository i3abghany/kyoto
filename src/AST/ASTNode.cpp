#include <cassert>
#include <fmt/core.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/ExpressionNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"

namespace llvm {
class LLVMContext;
}

llvm::Type* ASTNode::get_llvm_type(const KType* type, llvm::LLVMContext& context)
{
    auto primitive_type = dynamic_cast<const PrimitiveType*>(type);
    if (!primitive_type) {
        auto ptr_type = dynamic_cast<const PointerType*>(type);
        if (!ptr_type) {
            throw std::runtime_error(fmt::format("Unsupported type `{}`", type->to_string()));
        }
        return llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0);
    }

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
    case PrimitiveType::Kind::String:
        return llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0);
    case PrimitiveType::Kind::Unknown:
        assert(false && "Unsupported type");
    }
    return nullptr;
}

ProgramNode::ProgramNode(std::vector<ASTNode*> nodes, ModuleCompiler& compiler)
    : nodes(std::move(nodes))
    , compiler(compiler)
{
}

ProgramNode::~ProgramNode()
{
    for (auto* node : nodes)
        delete node;
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

ExpressionStatementNode::ExpressionStatementNode(ExpressionNode* expr, ModuleCompiler& compiler)
    : expr(expr)
    , compiler(compiler)
{
}

ExpressionStatementNode::~ExpressionStatementNode()
{
    delete expr;
}

std::string ExpressionStatementNode::to_string() const
{
    return fmt::format("ExpressionStatementNode({})", expr->to_string());
}

llvm::Value* ExpressionStatementNode::gen()
{
    return expr->gen();
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

    auto* expr_ktype = expr ? KType::from_llvm_type(expr->get_type(compiler.get_context())) : KType::get_void();
    auto fn_name = compiler.get_current_function_node()->get_name();

    llvm::Value* expr_val = nullptr;
    if (fn_ret_type->is_void() && !expr) {
        return compiler.get_builder().CreateRetVoid();
    } else if (fn_ret_type->is_integer() && expr_ktype->is_integer()
               || fn_ret_type->is_boolean() && expr_ktype->is_boolean()) {
        expr_val = ExpressionNode::handle_integer_conversion(expr, fn_ret_type, compiler, "return", fn_name);
    } else if (fn_ret_type->is_string() && expr_ktype->is_string()) {
        expr_val = expr->gen();
    } else {
        throw std::runtime_error(fmt::format(
            "Type of expression `{}` (type: `{}`) can't be returned from the function `{}`. Expected type `{}`",
            expr->to_string(), expr_ktype->to_string(), fn_name, fn_ret_type->to_string()));
    }

    return compiler.get_builder().CreateRet(expr_val);
}

BlockNode::BlockNode(std::vector<ASTNode*> nodes, ModuleCompiler& compiler)
    : nodes(std::move(nodes))
    , compiler(compiler)
{
}

BlockNode::~BlockNode()
{
    for (auto* node : nodes)
        delete node;
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
    if (nodes.empty()) return nullptr;

    compiler.push_scope();

    for (auto* node : nodes) {
        node->gen();
    }

    compiler.pop_scope();
    return nullptr;
}

FunctionNode::FunctionNode(const std::string& name, std::vector<Parameter> args, bool varargs, KType* ret_type,
                           ASTNode* body, ModuleCompiler& compiler)
    : name(name)
    , args(std::move(args))
    , varargs(varargs)
    , ret_type(ret_type)
    , body(body)
    , compiler(compiler)
{
}

FunctionNode::~FunctionNode()
{
    // the void type is a singleton, so we don't need to delete it
    if (!ret_type->is_void()) delete ret_type;
    delete body;
    for (auto& arg : args)
        delete arg.type;
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
    auto* func_type = llvm::FunctionType::get(return_ltype, get_arg_types(), varargs);
    auto* func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, name, compiler.get_module());

    if (body == nullptr) return func;

    auto* entry = llvm::BasicBlock::Create(compiler.get_context(), "func_entry", func);
    compiler.get_builder().SetInsertPoint(entry);

    compiler.push_fn_return_type(ret_type);
    compiler.set_current_function(this, func);
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