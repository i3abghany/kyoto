#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fmt/core.h>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/ModuleCompiler.h"

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"

namespace llvm {
class LLVMContext;
}

llvm::Type* ASTNode::get_type(const std::string& type, llvm::LLVMContext& context)
{
    if (type == "i8") {
        return llvm::Type::getInt8Ty(context);
    } else if (type == "i16") {
        return llvm::Type::getInt16Ty(context);
    } else if (type == "i32") {
        return llvm::Type::getInt32Ty(context);
    } else if (type == "i64") {
        return llvm::Type::getInt64Ty(context);
    } else {
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

IntNode::IntNode(int64_t value, size_t width, ModuleCompiler& compiler)
    : value(value)
    , width(width)
    , compiler(compiler)
{
}

std::string IntNode::to_string() const { return fmt::format("{}Node({})", get_type(), value); }

llvm::Value* IntNode::gen()
{
    return llvm::ConstantInt::get(compiler.get_context(), llvm::APInt(width * 8, value, true));
}

std::string_view IntNode::get_type() const
{
    switch (width) {
    case 8:
        return "I8";
    case 16:
        return "I16";
    case 32:
        return "I32";
    case 64:
        return "I64";
    default:
        return "I64";
    }
}

IdentifierExpressionNode::IdentifierExpressionNode(std::string name, ModuleCompiler& compiler)
    : name(name)
    , compiler(compiler)
{
}

std::string IdentifierExpressionNode::to_string() const { return fmt::format("IdentifierNode({})", name); }

llvm::Value* IdentifierExpressionNode::gen()
{
    auto symbol = compiler.get_symbol(name);
    if (!symbol.has_value()) {
        assert(false && "Unknown symbol");
    }

    return compiler.get_builder().CreateLoad(symbol.value()->getAllocatedType(), symbol.value(), name);
}

DeclarationStatementNode::DeclarationStatementNode(std::string name, std::string type, ModuleCompiler& compiler)
    : name(name)
    , type(type)
    , compiler(compiler)
{
}

std::string DeclarationStatementNode::to_string() const { return fmt::format("DeclarationNode({}, {})", name, type); }

llvm::Value* DeclarationStatementNode::gen()
{
    auto* type = get_type(this->type, compiler.get_context());
    auto* val = new llvm::AllocaInst(type, 0, name, compiler.get_builder().GetInsertBlock());
    compiler.add_symbol(name, val);
    return val;
}

FullDeclarationStatementNode::FullDeclarationStatementNode(std::string name, std::string type, ASTNode* expr,
                                                           ModuleCompiler& compiler)
    : name(name)
    , type(type)
    , expr(expr)
    , compiler(compiler)
{
}

std::string FullDeclarationStatementNode::to_string() const
{
    return fmt::format("FullDeclarationNode({}, {}, {})", name, type, expr->to_string());
}

llvm::Value* FullDeclarationStatementNode::gen()
{
    auto* type = get_type(this->type, compiler.get_context());
    auto* alloca = new llvm::AllocaInst(type, 0, name, compiler.get_builder().GetInsertBlock());
    auto* expr_val = expr->gen();
    compiler.get_builder().CreateStore(expr_val, alloca);
    compiler.add_symbol(name, alloca);
    return alloca;
}

ReturnStatementNode::ReturnStatementNode(ASTNode* expr, ModuleCompiler& compiler)
    : expr(expr)
    , compiler(compiler)
{
}

std::string ReturnStatementNode::to_string() const { return fmt::format("ReturnNode({})", expr->to_string()); }

llvm::Value* ReturnStatementNode::gen()
{
    auto* expr_val = expr->gen();
    return compiler.get_builder().CreateRet(expr_val);
}

UnaryNode::UnaryNode(ASTNode* expr, std::string op, ModuleCompiler& compiler)
    : expr(expr)
    , op(op)
    , compiler(compiler)
{
}

std::string UnaryNode::to_string() const { return fmt::format("UnaryNode({}, {})", op, expr->to_string()); }

llvm::Value* UnaryNode::gen()
{
    auto* expr_val = expr->gen();
    if (op == "-") {
        return compiler.get_builder().CreateNeg(expr_val, "negval");
    } else if (op == "+") {
        return expr_val;
    }
    return nullptr;
}

FunctionNode::FunctionNode(const std::string& name, std::vector<Parameter> args, std::string ret_type,
                           std::vector<ASTNode*> body, ModuleCompiler& compiler)
    : name(name)
    , args(std::move(args))
    , ret_type(std::move(ret_type))
    , body(std::move(body))
    , compiler(compiler)
{
}

std::string FunctionNode::to_string() const
{
    std::string args_str;
    for (const auto& arg : args) {
        args_str += arg.name + ": " + arg.type + ", ";
    }
    std::string body_str;
    for (const auto* node : body) {
        body_str += node->to_string() + ", ";
    }
    return fmt::format("FunctionNode({}, [{}], [{}])", name, args_str, body_str);
}

llvm::Value* FunctionNode::gen()
{
    auto* return_type = get_type(ret_type, compiler.get_context());
    auto* func_type = llvm::FunctionType::get(return_type, get_arg_types(), false);
    auto* func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, name, compiler.get_module());
    auto* entry = llvm::BasicBlock::Create(compiler.get_context(), "func_entry", func);
    compiler.get_builder().SetInsertPoint(entry);

    for (auto* node : body) {
        node->gen();
    }

    return func;
}

std::vector<llvm::Type*> FunctionNode::get_arg_types() const
{
    std::vector<llvm::Type*> types;
    for (const auto& arg : args) {
        auto* type = get_type(arg.type, compiler.get_context());
        types.push_back(type);
    }
    return types;
}