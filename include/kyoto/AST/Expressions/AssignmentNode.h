#pragma once

#include <string>
#include <vector>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/Expressions/ExpressionNode.h"

class ModuleCompiler;
namespace llvm {
class Value;
} // namespace llvm

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

private:
    ExpressionNode* assignee;
    ExpressionNode* expr;
    ModuleCompiler& compiler;
};