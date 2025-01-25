#pragma once

#include <string>
#include <vector>

#include "kyoto/AST/Expressions/ExpressionNode.h"

class ModuleCompiler;

class IdentifierExpressionNode : public ExpressionNode {
public:
    IdentifierExpressionNode(std::string name, ModuleCompiler& compiler);
    ~IdentifierExpressionNode() override = default;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Value* gen_ptr() const override;
    [[nodiscard]] llvm::Type* gen_type() const override;
    [[nodiscard]] KType* get_ktype() const override;

    [[nodiscard]] std::vector<ASTNode*> get_children() const override { return {}; }

    [[nodiscard]] const std::string& get_name() const { return name; }

private:
    std::string name;
    ModuleCompiler& compiler;
};