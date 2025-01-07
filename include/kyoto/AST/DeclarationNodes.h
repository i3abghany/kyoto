#pragma once

#include <string>

#include "kyoto/AST/ASTNode.h"

class ModuleCompiler;
class KType;

class DeclarationStatementNode : public ASTNode {
    std::string name;
    KType* type;
    ModuleCompiler& compiler;

public:
    DeclarationStatementNode(std::string name, KType* ktype, ModuleCompiler& compiler);
    ~DeclarationStatementNode();

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
};

class FullDeclarationStatementNode : public ASTNode {
    std::string name;
    KType* type;
    ExpressionNode* expr;
    ModuleCompiler& compiler;

public:
    FullDeclarationStatementNode(std::string name, KType* type, ExpressionNode* expr, ModuleCompiler& compiler);
    ~FullDeclarationStatementNode();

    [[nodiscard]] std::string to_string() const override;
    llvm::Value* gen() override;
};
