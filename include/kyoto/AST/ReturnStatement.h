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

private:
    void validate_void_return() const;
    llvm::Value* generate_return_value() const;
    bool are_compatible_integers_or_booleans() const;
    bool are_compatible_pointer_types() const;
    llvm::Value* generate_pointer_return_value() const;
};
