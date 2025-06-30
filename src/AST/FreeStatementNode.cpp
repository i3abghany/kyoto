#include "kyoto/AST/FreeStatementNode.h"

#include <format>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <vector>

#include "kyoto/AST/Expressions/ExpressionNode.h"
#include "kyoto/ModuleCompiler.h"

FreeStatementNode::FreeStatementNode(ExpressionNode* expr, ModuleCompiler& compiler)
    : expr(expr)
    , compiler(compiler)
{
}

FreeStatementNode::~FreeStatementNode()
{
    delete expr;
}

std::string FreeStatementNode::to_string() const
{
    return std::format("Free({})", expr->to_string());
}

llvm::Value* FreeStatementNode::gen()
{
    auto* ptr = expr->gen();
    auto* free_fn = compiler.get_module()->getFunction("free");
    llvm::ArrayRef<llvm::Value*> args = { ptr };
    return compiler.get_builder().CreateCall(free_fn, args);
}

std::vector<ASTNode*> FreeStatementNode::get_children() const
{
    return { expr };
}