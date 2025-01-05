#pragma once

#include <string>

#include "kyoto/AST/ASTNode.h"

class ModuleCompiler;

class IfStatementNode : public ASTNode {
public:
    IfStatementNode(ExpressionNode* condition, ASTNode* then_node, ASTNode* else_node, ModuleCompiler& compiler);
    ~IfStatementNode();

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;

private:
    ExpressionNode* condition;
    ASTNode* then;
    ASTNode* els;
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