#include "kyoto/AST/ControlFlowNodes.h"

#include <fmt/core.h>
#include <stddef.h>
#include <stdexcept>
#include <utility>

#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"

IfStatementNode::IfStatementNode(std::vector<ExpressionNode*> conditions, std::vector<ASTNode*> bodies,
                                 ModuleCompiler& compiler)
    : conditions(std::move(conditions))
    , bodies(std::move(bodies))
    , compiler(compiler)
{
}

IfStatementNode::~IfStatementNode()
{
    for (auto* cond : conditions)
        delete cond;
    for (auto* body : bodies)
        delete body;
}

std::string IfStatementNode::to_string() const
{
    std::string str;

    for (size_t i = 0; i < conditions.size(); ++i) {
        str += fmt::format("IfStatement({}, {})", conditions[i]->to_string(), bodies[i]->to_string());
    }

    if (bodies.size() > conditions.size()) {
        str += fmt::format("ElseStatement({})", bodies.back()->to_string());
    }

    return fmt::format("IfStatement([{}])", str);
}

llvm::Value* IfStatementNode::gen()
{
    llvm::BasicBlock* current_bb = compiler.get_builder().GetInsertBlock();

    // Blocks that we unconditionally jump to one after one (condition
    // evaluation blocks and the else block)
    std::vector<llvm::BasicBlock*> jumps_bbs;

    // Blocks that contain the body of the if statements
    std::vector<llvm::BasicBlock*> body_bbs;

    for (size_t i = 0; i < conditions.size(); i++) {
        auto* cond_bb = compiler.create_basic_block("if_cond");
        jumps_bbs.push_back(cond_bb);

        auto* body_bb = compiler.create_basic_block("if_body");
        body_bbs.push_back(body_bb);
    }

    if (has_else()) {
        auto* else_bb = compiler.create_basic_block("else_body");
        jumps_bbs.push_back(else_bb);
    }

    auto* end_bb = compiler.create_basic_block("if_end");
    jumps_bbs.push_back(end_bb);

    compiler.get_builder().SetInsertPoint(current_bb);
    compiler.get_builder().CreateBr(jumps_bbs[0]);

    for (size_t i = 0; i < body_bbs.size(); i++) {
        auto* cond_type = conditions[i]->get_type(compiler.get_context());
        auto cond_ktype = PrimitiveType::from_llvm_type(cond_type);

        if (cond_ktype.get_kind() != PrimitiveType::Kind::Boolean) {
            throw std::runtime_error(fmt::format("If condition must be of type bool, got {}", cond_ktype.to_string()));
        }

        compiler.get_builder().SetInsertPoint(jumps_bbs[i]);
        auto* cond_val = conditions[i]->gen();
        compiler.get_builder().CreateCondBr(cond_val, body_bbs[i], jumps_bbs[i + 1]);
    }

    if (has_else()) body_bbs.push_back(jumps_bbs[jumps_bbs.size() - 2]);

    for (size_t i = 0; i < body_bbs.size(); i++) {
        compiler.get_builder().SetInsertPoint(body_bbs[i]);
        bodies[i]->gen();
        if (compiler.get_builder().GetInsertBlock()->empty()
            || !compiler.get_builder().GetInsertBlock()->back().isTerminator()) {
            compiler.get_builder().CreateBr(end_bb);
        }
    }

    compiler.get_builder().SetInsertPoint(end_bb);
    return nullptr;
}

ForStatementNode::ForStatementNode(ASTNode* init, ExpressionStatementNode* condition, ExpressionNode* update,
                                   ASTNode* body, ModuleCompiler& compiler)
    : init(init)
    , condition(condition)
    , update(update)
    , body(body)
    , compiler(compiler)
{
}

ForStatementNode::~ForStatementNode()
{
    delete init;
    delete condition;
    delete update;
    delete body;
}

std::string ForStatementNode::to_string() const
{
    return fmt::format("ForStatement({}, {}, {}, {})", init->to_string(), condition->to_string(), update->to_string(),
                       body->to_string());
}

llvm::Value* ForStatementNode::gen()
{
    return nullptr;
}