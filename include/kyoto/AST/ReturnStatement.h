#pragma once

#include <string>
#include <vector>

#include "kyoto/AST/ASTNode.h"

class ModuleCompiler;
class ExpressionNode;

class ReturnStatementNode : public ASTNode {
    ExpressionNode* expr;
    ModuleCompiler& compiler;

public:
    ReturnStatementNode(ExpressionNode* expr, ModuleCompiler& compiler);
    ~ReturnStatementNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] std::vector<ASTNode*> get_children() const override;
};
