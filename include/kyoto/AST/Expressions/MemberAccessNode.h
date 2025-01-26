#pragma once

#include "kyoto/AST/Expressions/ExpressionNode.h"

#include <string>
#include <vector>

namespace llvm {
class Value;
class Type;
}

class ModuleCompiler;

class MemberAccessNode : public ExpressionNode {
public:
    MemberAccessNode(ExpressionNode* lhs, std::string member, ModuleCompiler& compiler);
    ~MemberAccessNode();

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Value* gen_ptr() const override;
    [[nodiscard]] llvm::Type* gen_type() const override;
    [[nodiscard]] KType* get_ktype() const override;
    [[nodiscard]] llvm::Value* trivial_gen() override;
    [[nodiscard]] bool is_trivially_evaluable() const override;

    [[nodiscard]] const std::string& get_member() const { return member; }
    [[nodiscard]] ExpressionNode* get_lhs() const { return lhs; }

    [[nodiscard]] std::vector<ASTNode*> get_children() const override { return { lhs }; }

private:
    ExpressionNode* lhs;
    std::string member;
    ModuleCompiler& compiler;
};