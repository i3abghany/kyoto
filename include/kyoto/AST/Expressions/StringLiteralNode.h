#pragma once

#include <string>
#include <vector>

#include "kyoto/AST/Expressions/ExpressionNode.h"
#include "kyoto/KType.h"

class ModuleCompiler;

class StringLiteralNode final : public ExpressionNode {
public:
    StringLiteralNode(std::string value, ModuleCompiler& compiler);
    ~StringLiteralNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Type* gen_type() const override;
    [[nodiscard]] KType* get_ktype() const override;
    [[nodiscard]] llvm::Value* trivial_gen() override;
    [[nodiscard]] bool is_trivially_evaluable() const override;

    [[nodiscard]] std::vector<ASTNode*> get_children() const override { return {}; }

    void cast_to(PrimitiveType::Kind target_type);

private:
    mutable KType* type;
    std::string value;
    ModuleCompiler& compiler;
};