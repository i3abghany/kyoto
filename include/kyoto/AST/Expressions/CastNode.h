#pragma once

#include <string>
#include <vector>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/Expressions/ExpressionNode.h"

class KType;
class ModuleCompiler;

class CastNode : public ExpressionNode {
public:
    CastNode(KType* type, ExpressionNode* expr, ModuleCompiler& compiler);
    ~CastNode();

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Value* gen_ptr() const override;
    [[nodiscard]] llvm::Type* gen_type() const override;
    [[nodiscard]] KType* get_ktype() const override;
    [[nodiscard]] llvm::Value* trivial_gen() override;
    [[nodiscard]] bool is_trivially_evaluable() const override;

    [[nodiscard]] std::vector<ASTNode*> get_children() const override { return { expr }; }

    [[nodiscard]] KType* get_type() const { return type; }
    [[nodiscard]] ExpressionNode* get_expr() const { return expr; }

private:
    llvm::Value* handle_integer_cast();
    void check_compatible_integer_cast(const PrimitiveType* expr_ktype, const PrimitiveType* target_type);
    void throw_incompatible_cast_error(const KType* expr_ktype, const KType* target_ktype) const;

private:
    KType* type;
    ExpressionNode* expr;
    ModuleCompiler& compiler;
};
