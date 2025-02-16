#include "kyoto/AST/Expressions/MatchNode.h"

#include <fmt/core.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <stddef.h>
#include <stdexcept>
#include <utility>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"

namespace llvm {
class Value;
}

MatchNode::MatchNode(ExpressionNode* expr, std::vector<Case> cases, ModuleCompiler& compiler)
    : expr(expr)
    , cases(std::move(cases))
    , compiler(compiler)
{
    validate_default();
    check_types();
    init_ktype();
}

std::string MatchNode::to_string() const
{
    std::string str = "match " + expr->to_string() + " {\n";
    for (const auto& c : cases) {
        str += "case " + c.cond->to_string() + " => " + c.ret->to_string() + "\n";
    }
    str += "}";
    return str;
}

llvm::Value* MatchNode::gen_cmp(ExpressionNode* lhs, ExpressionNode* rhs)
{
    if (lhs->get_ktype()->is_integer() || lhs->get_ktype()->is_pointer())
        return compiler.get_builder().CreateICmpEQ(lhs->gen(), rhs->gen(), "cmp_match");

    throw std::runtime_error(fmt::format("Unsupported type in match expression: `{}`", lhs->get_ktype()->to_string()));
}

llvm::Value* MatchNode::gen()
{
    if (cases.size() == 1) return gen_default_only();

    auto* match_expr_val = expr->gen();

    std::vector<llvm::BasicBlock*> case_bbs(cases.size() - 1);
    for (size_t i = 0; i < cases.size() - 1; ++i) {
        case_bbs[i] = compiler.create_basic_block(fmt::format("case_{}", i));
    }

    auto* default_bb = compiler.create_basic_block("default");
    auto* merge_bb = compiler.create_basic_block("merge");

    llvm::BasicBlock* comparison_start_bb = compiler.get_builder().GetInsertBlock();

    compiler.get_builder().SetInsertPoint(merge_bb);
    auto* phi = compiler.get_builder().CreatePHI(cases.back().ret->gen_type(), cases.size());

    for (size_t i = 0; i < cases.size() - 1; ++i) {
        llvm::BasicBlock* case_block = case_bbs[i];
        compiler.get_builder().SetInsertPoint(comparison_start_bb);

        llvm::Value* cmp = gen_cmp(cases[i].cond, expr);
        llvm::BasicBlock* next_cmp_start_bb
            = (i < cases.size() - 2) ? compiler.create_basic_block(fmt::format("case_cmp_next_{}", i + 1)) : default_bb;

        compiler.get_builder().CreateCondBr(cmp, case_block, next_cmp_start_bb);

        compiler.get_builder().SetInsertPoint(case_block);
        auto* case_ret = cases[i].ret->gen();
        compiler.get_builder().CreateBr(merge_bb);
        phi->addIncoming(case_ret, case_block);

        if (i < cases.size() - 2) compiler.get_builder().SetInsertPoint(next_cmp_start_bb);
        comparison_start_bb = next_cmp_start_bb;
    }

    compiler.get_builder().SetInsertPoint(default_bb);
    auto default_ret = cases.back().ret->gen();
    compiler.get_builder().CreateBr(merge_bb);
    phi->addIncoming(default_ret, default_bb);

    compiler.get_builder().SetInsertPoint(merge_bb);

    return phi;
}

llvm::Value* MatchNode::gen_default_only()
{
    return cases.back().ret->gen();
}

llvm::Type* MatchNode::gen_type() const
{
    return cases.back().ret->gen_type();
}

std::vector<ASTNode*> MatchNode::get_children() const
{
    std::vector<ASTNode*> children;
    children.push_back(expr);
    for (const auto& c : cases) {
        if (c.cond) children.push_back(c.cond);
        children.push_back(c.ret);
    }
    return children;
}

void MatchNode::check_types() const
{
    auto* def_type = cases.back().ret->get_ktype();
    for (size_t i = 0; i < cases.size() - 1; i++) {
        auto* case_type = cases[i].ret->get_ktype();
        if (*def_type != *case_type) {
            throw std::runtime_error(fmt::format(
                "Type mismatch in match case expression: `{}` (type `{}`), `{}` (type `{}`)", cases[i].ret->to_string(),
                case_type->to_string(), cases.back().ret->to_string(), def_type->to_string()));
        }
    }
}

void MatchNode::validate_default()
{
    size_t default_case = 0xFFFF;
    for (size_t i = 0; i < cases.size(); i++) {
        if (!cases[i].cond) {
            if (default_case != 0xFFFF)
                throw std::runtime_error("Encountered multiple default cases in match expression");
            default_case = i;
        }
    }
    if (default_case == 0xFFFF) throw std::runtime_error("Match expression must have a default case");
    std::swap(cases[default_case], cases.back());
}

void MatchNode::init_ktype()
{
    type = cases.back().ret->get_ktype();
}