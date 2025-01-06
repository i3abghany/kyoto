#pragma once

#include <string>
#include <vector>

#include "kyoto/AST/ASTNode.h"

class ModuleCompiler;

class IfStatementNode : public ASTNode {
public:
    IfStatementNode(std::vector<ExpressionNode*> conditions, std::vector<ASTNode*> bodies, ModuleCompiler& compiler);
    ~IfStatementNode();

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;

    bool has_else() const { return conditions.size() != bodies.size(); }

private:
    std::vector<ExpressionNode*> conditions;
    std::vector<ASTNode*> bodies;
    ModuleCompiler& compiler;
};

class ForStatementNode : public ASTNode {
public:
    ForStatementNode(ASTNode* init, ExpressionStatementNode* condition, ExpressionNode* update, ASTNode* body,
                     ModuleCompiler& compiler);
    ~ForStatementNode();

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;

private:
    ASTNode* init;
    ExpressionStatementNode* condition;
    ExpressionNode* update;
    ASTNode* body;
    ModuleCompiler& compiler;
};