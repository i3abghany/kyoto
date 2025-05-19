#pragma once

#include <string>
#include <vector>
#include <format>

#include "kyoto/AST/Expressions/ExpressionNode.h"

class KType;
class ModuleCompiler;

namespace llvm {
class Value;
}

class MatchNode : public ExpressionNode {
public:
    struct Case {
        ExpressionNode* cond;
        ExpressionNode* ret;
    };

public:
    MatchNode(ExpressionNode* expr, std::vector<Case> cases, ModuleCompiler& compiler);

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Type* gen_type() const override;
    [[nodiscard]] KType* get_ktype() const override;
    [[nodiscard]] std::vector<ASTNode*> get_children() const override;

    [[nodiscard]] llvm::Value* gen_default_only();

private:
    void check_types() const;
    void validate_default();
    void init_ktype() const;
    [[nodiscard]] llvm::Value* gen_cmp(ExpressionNode* lhs, ExpressionNode* rhs);

private:
    ExpressionNode* expr;
    std::vector<Case> cases;
    KType* type;
    ModuleCompiler& compiler;
};