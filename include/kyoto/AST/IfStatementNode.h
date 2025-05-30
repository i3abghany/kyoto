#pragma once

#include <string>
#include <vector>

#include "kyoto/AST/ASTNode.h"

class ModuleCompiler;
class ExpressionNode;

class IfStatementNode final : public ASTNode {
public:
    IfStatementNode(std::vector<ExpressionNode*> conditions, std::vector<ASTNode*> bodies, ModuleCompiler& compiler);
    ~IfStatementNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;

    [[nodiscard]] std::vector<ASTNode*> get_children() const override;

    bool has_else() const { return conditions.size() != bodies.size(); }

private:
    std::vector<ExpressionNode*> conditions;
    std::vector<ASTNode*> bodies;
    ModuleCompiler& compiler;
};