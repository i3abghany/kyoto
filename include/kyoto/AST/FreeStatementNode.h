#pragma once

#include <string>

#include "kyoto/AST/ASTNode.h"

class ExpressionNode;
class ModuleCompiler;

class FreeStatementNode : public ASTNode {
    ExpressionNode* expr;
    ModuleCompiler& compiler;

public:
    FreeStatementNode(ExpressionNode* expr, ModuleCompiler& compiler);
    ~FreeStatementNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] std::vector<ASTNode*> get_children() const override;
};