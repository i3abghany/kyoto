#pragma once

#include <string>
#include <vector>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/Expressions/ExpressionNode.h"

class ModuleCompiler;
class KType;

namespace llvm {
class Value;
}

class Symbol;

class AssignmentNode : public ExpressionNode {
public:
    AssignmentNode(ExpressionNode* assignee, ExpressionNode* expr, ModuleCompiler& compiler);
    ~AssignmentNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Type* gen_type() const override;
    [[nodiscard]] llvm::Value* trivial_gen() override;
    [[nodiscard]] KType* get_ktype() const override;
    [[nodiscard]] bool is_trivially_evaluable() const override;

    [[nodiscard]] std::vector<ASTNode*> get_children() const override { return { assignee, expr }; }

private:
    [[nodiscard]] llvm::Value* gen_deref_assignment() const;
    void validate_lvalue() const;
    [[nodiscard]] Symbol get_lhs_lvalue() const;
    [[nodiscard]] llvm::Value* generate_expression_value(const KType* type, const std::string& name) const;
    [[nodiscard]] bool are_compatible_integers_or_booleans(const KType* type) const;
    [[nodiscard]] bool are_compatible_pointer_types(const KType* type) const;

private:
    ExpressionNode* assignee;
    ExpressionNode* expr;
    ModuleCompiler& compiler;
};