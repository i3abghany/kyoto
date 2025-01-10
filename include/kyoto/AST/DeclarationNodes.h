#pragma once

#include <string>

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
    llvm::Value* gen() override;

private:
    std::string name;
    KType* type;
    ExpressionNode* expr;
    ModuleCompiler& compiler;
};
