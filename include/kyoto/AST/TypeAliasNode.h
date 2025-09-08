#pragma once

#include <string>

#include "kyoto/AST/ASTNode.h"

class ModuleCompiler;
class KType;

class TypeAliasNode final : public ASTNode {
public:
    TypeAliasNode(KType* originalType, std::string alias, ModuleCompiler& compiler);
    ~TypeAliasNode() override = default;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] std::vector<ASTNode*> get_children() const override { return {}; }

    [[nodiscard]] KType* get_original_type() const { return originalType; }
    [[nodiscard]] std::string get_alias() const { return alias; }

private:
    KType* originalType;
    std::string alias;
    ModuleCompiler& compiler;
};
