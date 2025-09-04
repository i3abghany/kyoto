#pragma once

#include <string>
#include <vector>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/Expressions/ExpressionNode.h"

class KType;
class ModuleCompiler;

class NewArrayNode : public ExpressionNode {
public:
    NewArrayNode(KType* type, ExpressionNode* size_expr, ModuleCompiler& compiler);

    ~NewArrayNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Value* gen_ptr() const override;
    [[nodiscard]] llvm::Type* gen_type() const override;
    [[nodiscard]] KType* get_ktype() const override;
    [[nodiscard]] llvm::Value* trivial_gen() override;
    [[nodiscard]] bool is_trivially_evaluable() const override;

    [[nodiscard]] KType* get_type() const { return type; }

    [[nodiscard]] KType* get_generated_type() const { return generated_type; }

    [[nodiscard]] ExpressionNode* get_size_expr() const { return size_expr; }

    [[nodiscard]] std::vector<ASTNode*> get_children() const override { return { size_expr }; }

private:
    KType* type;
    KType* generated_type;
    ExpressionNode* size_expr;
    ModuleCompiler& compiler;
};