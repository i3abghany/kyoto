#pragma once

#include <fmt/core.h>
#include <iostream>

#include "llvm/IR/Type.h"

class ModuleCompiler;
namespace llvm {
class LLVMContext;
}
namespace llvm {
class Value;
}

class ASTNode {
public:
    virtual ~ASTNode() = default;
    [[nodiscard]] virtual std::string to_string() const = 0;
    virtual llvm::Value* gen() = 0;

protected:
    static llvm::Type* get_type(const std::string& type, llvm::LLVMContext& context);
};

class ProgramNode : public ASTNode {
    std::vector<ASTNode*> nodes;
    ModuleCompiler& compiler;

public:
    ProgramNode(std::vector<ASTNode*> nodes, ModuleCompiler& compiler);

    [[nodiscard]] std::string to_string() const override;
    llvm::Value* gen() override;
};

class IdentifierExpressionNode : public ASTNode {
    std::string name;
    ModuleCompiler& compiler;

public:
    IdentifierExpressionNode(std::string name, ModuleCompiler& compiler);

    [[nodiscard]] std::string to_string() const override;
    llvm::Value* gen() override;
};

class DeclarationStatementNode : public ASTNode {
    std::string name;
    std::string type;
    ModuleCompiler& compiler;

public:
    DeclarationStatementNode(std::string name, std::string type, ModuleCompiler& compiler);

    [[nodiscard]] std::string to_string() const override;
    llvm::Value* gen() override;
};

class FullDeclarationStatementNode : public ASTNode {
    std::string name;
    std::string type;
    ASTNode* expr;
    ModuleCompiler& compiler;

public:
    FullDeclarationStatementNode(std::string name, std::string type, ASTNode* expr, ModuleCompiler& compiler);

    [[nodiscard]] std::string to_string() const override;
    llvm::Value* gen() override;
};

class ReturnStatementNode : public ASTNode {
    ASTNode* expr;
    ModuleCompiler& compiler;

public:
    ReturnStatementNode(ASTNode* expr, ModuleCompiler& compiler);

    [[nodiscard]] std::string to_string() const override;
    llvm::Value* gen() override;
};

class UnaryNode : public ASTNode {
    ASTNode* expr;
    std::string op;
    ModuleCompiler& compiler;

public:
    UnaryNode(ASTNode* expr, std::string op, ModuleCompiler& compiler);

    [[nodiscard]] std::string to_string() const override;
    llvm::Value* gen() override;
};

class FunctionNode : public ASTNode {
public:
    struct Parameter {
        std::string name;
        std::string type;
    };

    FunctionNode(const std::string& name, std::vector<Parameter> args, std::string ret_type, std::vector<ASTNode*> body,
                 ModuleCompiler& compiler);

    [[nodiscard]] std::string to_string() const override;
    llvm::Value* gen() override;

private:
    [[nodiscard]] std::vector<llvm::Type*> get_arg_types() const;

    std::string name;
    std::vector<Parameter> args;
    std::string ret_type;
    std::vector<ASTNode*> body;
    ModuleCompiler& compiler;
};