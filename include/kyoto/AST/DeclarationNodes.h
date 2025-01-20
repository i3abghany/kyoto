#pragma once

#include <string>
#include <vector>

#include "kyoto/AST/ASTNode.h"

class ModuleCompiler;
class KType;
class ExpressionNode;

class DeclarationStatementNode final : public ASTNode {
public:
    DeclarationStatementNode(std::string name, KType* ktype, ModuleCompiler& compiler);
    ~DeclarationStatementNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;

    [[nodiscard]] KType* get_ktype() const { return type; }
    [[nodiscard]] std::string get_name() const { return name; }
    [[nodiscard]] std::vector<ASTNode*> get_children() const override { return {}; }

private:
    std::string name;
    KType* type;
    ModuleCompiler& compiler;
};

class FullDeclarationStatementNode final : public ASTNode {
public:
    FullDeclarationStatementNode(std::string name, KType* type, ExpressionNode* expr, ModuleCompiler& compiler);
    ~FullDeclarationStatementNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] std::vector<ASTNode*> get_children() const override;

private:
    std::string name;
    KType* type;
    ExpressionNode* expr;
    ModuleCompiler& compiler;
};
