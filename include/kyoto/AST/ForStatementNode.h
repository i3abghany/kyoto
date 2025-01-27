#pragma once

#include <string>

#include "kyoto/AST/ASTNode.h"

class ModuleCompiler;
class ExpressionNode;

namespace llvm {
class BasicBlock;
}

class ForStatementNode : public ASTNode {
public:
    ForStatementNode(ASTNode* init, ExpressionStatementNode* condition, ExpressionNode* update, ASTNode* body,
                     ModuleCompiler& compiler);
    ~ForStatementNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;

    [[nodiscard]] std::vector<ASTNode*> get_children() const override;

private:
    void handle_init() const;
    void handle_condition(llvm::BasicBlock* cond_bb, llvm::BasicBlock* body_bb, llvm::BasicBlock* out_bb) const;
    bool handle_trivial_false_condition(llvm::BasicBlock* out_bb) const;
    void handle_update(llvm::BasicBlock* update_bb, llvm::BasicBlock* cond_bb, llvm::BasicBlock* body_bb) const;
    void verify_bool_condition() const;

private:
    ASTNode* init;
    ExpressionStatementNode* condition;
    ExpressionNode* update;
    ASTNode* body;
    ModuleCompiler& compiler;
};