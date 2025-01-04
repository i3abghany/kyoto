#include "kyoto/AST/ControlFlowNodes.h"

#include <fmt/core.h>
#include <stdexcept>

#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "llvm/ADT/ilist_iterator.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"

IfStatementNode::IfStatementNode(ExpressionNode* condition, ASTNode* then_node, ASTNode* else_node,
                                 ModuleCompiler& compiler)
    : condition(condition)
    , then(then_node)
    , els(else_node)
    , compiler(compiler)
{
}

IfStatementNode::~IfStatementNode()
{
    delete condition;
    delete then;
    if (els) delete els;
}

std::string IfStatementNode::to_string() const
{
    return fmt::format("IfStatement({}, {}, {})", condition->to_string(), then->to_string(), els->to_string());
}

llvm::Value* IfStatementNode::gen()
{
    auto* cond_type = condition->get_type(compiler.get_context());
    auto cond_ktype = PrimitiveType::from_llvm_type(cond_type);

    if (cond_ktype.get_kind() != PrimitiveType::Kind::Boolean) {
        throw std::runtime_error(fmt::format("If condition must be of type bool, got {}", cond_ktype.to_string()));
    }

    auto* cond_val = condition->gen();

    auto* fn = compiler.get_builder().GetInsertBlock()->getParent();
    auto* then_bb = llvm::BasicBlock::Create(compiler.get_context(), "then", fn);
    auto* else_bb = llvm::BasicBlock::Create(compiler.get_context(), "else");
    auto* merge_bb = llvm::BasicBlock::Create(compiler.get_context(), "ifcont");

    compiler.get_builder().CreateCondBr(cond_val, then_bb, else_bb);

    compiler.get_builder().SetInsertPoint(then_bb);
    then->gen();

    if (!then_bb->getTerminator()) compiler.get_builder().CreateBr(merge_bb);

    then_bb = compiler.get_builder().GetInsertBlock();

    fn->insert(fn->end(), else_bb);
    compiler.get_builder().SetInsertPoint(else_bb);

    if (els) els->gen();

    if (auto* second_if = dynamic_cast<IfStatementNode*>(els); second_if) return nullptr;

    if (!else_bb->getTerminator()) compiler.get_builder().CreateBr(merge_bb);

    else_bb = compiler.get_builder().GetInsertBlock();

    fn->insert(fn->end(), merge_bb);
    compiler.get_builder().SetInsertPoint(merge_bb);

    return nullptr;
}