#pragma once

#include <string>
#include <vector>

#include "kyoto/AST/ASTNode.h"

class ModuleCompiler;
class ExpressionNode;

class ForStatementNode : public ASTNode {
public:
    ForStatementNode(ASTNode* init, ExpressionStatementNode* condition, ExpressionNode* update, ASTNode* body,
                     ModuleCompiler& compiler);
    ~ForStatementNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;

    [[nodiscard]] std::vector<ASTNode*> get_children() const override;

private:
    ASTNode* init;
    ExpressionStatementNode* condition;
    ExpressionNode* update;
    ASTNode* body;
    ModuleCompiler& compiler;
};