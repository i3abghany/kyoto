#pragma once

#include <string>
#include <vector>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/Expressions/ExpressionNode.h"

namespace llvm {
class Type;
}

class ModuleCompiler;
class KType;
class ClassMetadata;

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
    void validate_member_access(KType* lhs_type) const;
    std::string get_class_name(KType* lhs_type) const;
    llvm::Type* get_class_type(KType* lhs_type) const;
    unsigned get_member_index(KType* lhs_type) const;
    const ASTNode* get_member_declaration(const ClassMetadata& class_metadata) const;

private:
    ExpressionNode* lhs;
    std::string member;
    ModuleCompiler& compiler;
};