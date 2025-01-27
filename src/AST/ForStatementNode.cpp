#include "kyoto/AST/ForStatementNode.h"

#include <fmt/core.h>
#include <stdexcept>
#include <vector>

#include "kyoto/AST/Expressions/ExpressionNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/Casting.h"

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
    auto* cond_bb = condition ? compiler.create_basic_block("for_cond") : nullptr;
    auto* body_bb = compiler.create_basic_block("for_body");
    auto* update_bb = update ? compiler.create_basic_block("for_update") : nullptr;
    auto* out_bb = compiler.create_basic_block("for_out");

    handle_init();
    verify_bool_condition();

    if (handle_trivial_false_condition(out_bb)) return nullptr;

    handle_condition(cond_bb, body_bb, out_bb);

    compiler.get_builder().SetInsertPoint(body_bb);
    body->gen();

    handle_update(update_bb, cond_bb, body_bb);

    compiler.get_builder().SetInsertPoint(out_bb);
    return nullptr;
}

void ForStatementNode::handle_init() const
{
    if (!init) return;
    auto* init_block = compiler.create_basic_block("for_init");
    compiler.get_builder().CreateBr(init_block);
    compiler.get_builder().SetInsertPoint(init_block);
    init->gen();
}

void ForStatementNode::handle_condition(llvm::BasicBlock* cond_bb, llvm::BasicBlock* body_bb,
                                        llvm::BasicBlock* out_bb) const
{
    compiler.get_builder().CreateBr(cond_bb ? cond_bb : body_bb);
    if (!condition) return;

    compiler.get_builder().SetInsertPoint(cond_bb);
    auto* cond_val = condition->gen();
    compiler.get_builder().CreateCondBr(cond_val, body_bb, out_bb);
}

void ForStatementNode::verify_bool_condition() const
{
    if (!condition) return;
    if (!condition->get_expr()->get_ktype()->is_boolean()) {
        throw std::runtime_error(
            fmt::format("For condition must be of type bool, got {}", condition->get_expr()->get_ktype()->to_string()));
    }
}

bool ForStatementNode::handle_trivial_false_condition(llvm::BasicBlock* out_bb) const
{
    if (!condition) return false;
    if (!condition->get_expr()->is_trivially_evaluable()) return false;

    auto* cond_val = condition->get_expr()->gen();
    auto* cond_ktype = condition->get_expr()->get_ktype();

    // if evaluates to false, set the insert point to the out block
    if (auto* constant_int = llvm::dyn_cast<llvm::ConstantInt>(cond_val)) {
        if (constant_int->isZero()) {
            compiler.get_builder().CreateBr(out_bb);
            compiler.get_builder().SetInsertPoint(out_bb);
            return true;
        }
    }

    return false;
}

void ForStatementNode::handle_update(llvm::BasicBlock* update_bb, llvm::BasicBlock* cond_bb,
                                     llvm::BasicBlock* body_bb) const
{
    if (!update) {
        compiler.get_builder().CreateBr(cond_bb ? cond_bb : body_bb);
        return;
    }

    compiler.get_builder().CreateBr(update_bb);
    compiler.get_builder().SetInsertPoint(update_bb);
    auto* _ = update->gen();
    compiler.get_builder().CreateBr(cond_bb);
}

std::vector<ASTNode*> ForStatementNode::get_children() const
{
    return { init, condition, update, body };
}