#pragma once

#include <fmt/core.h>
#include <iostream>

#include "llvm/IR/Type.h"

#include "kyoto/KType.h"

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
    static llvm::Type* get_llvm_type(const KType* type, llvm::LLVMContext& context);
};

class ExpressionNode : public ASTNode {
public:
    virtual ~ExpressionNode() = default;
    [[nodiscard]] virtual std::string to_string() const = 0;
    [[nodiscard]] virtual llvm::Value* gen() = 0;
    [[nodiscard]] virtual llvm::Type* get_type(llvm::LLVMContext& context) const = 0;
    [[nodiscard]] virtual llvm::Value* trivial_gen() { return nullptr; }
    [[nodiscard]] virtual bool is_trivially_evaluable() const { return false; }

    static llvm::Value* handle_integer_conversion(ExpressionNode* expr, KType* target_type, ModuleCompiler& compiler,
                                                  const std::string& what, const std::string& target_name = "");
};

class ProgramNode : public ASTNode {
    std::vector<ASTNode*> nodes;
    ModuleCompiler& compiler;

public:
    ProgramNode(std::vector<ASTNode*> nodes, ModuleCompiler& compiler);

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
};

class IdentifierExpressionNode : public ExpressionNode {
    std::string name;
    ModuleCompiler& compiler;

public:
    IdentifierExpressionNode(std::string name, ModuleCompiler& compiler);

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Type* get_type(llvm::LLVMContext& context) const override;
};

class DeclarationStatementNode : public ASTNode {
    std::string name;
    KType* type;
    ModuleCompiler& compiler;

public:
    DeclarationStatementNode(std::string name, KType* ktype, ModuleCompiler& compiler);

    [[nodiscard]] std::string to_string() const override;
    llvm::Value* gen() override;
};

class FullDeclarationStatementNode : public ASTNode {
    std::string name;
    KType* type;
    ExpressionNode* expr;
    ModuleCompiler& compiler;

public:
    FullDeclarationStatementNode(std::string name, KType* type, ExpressionNode* expr, ModuleCompiler& compiler);

    [[nodiscard]] std::string to_string() const override;
    llvm::Value* gen() override;
};

class ReturnStatementNode : public ASTNode {
    ExpressionNode* expr;
    ModuleCompiler& compiler;

public:
    ReturnStatementNode(ExpressionNode* expr, ModuleCompiler& compiler);

    [[nodiscard]] std::string to_string() const override;
    llvm::Value* gen() override;
};

class UnaryNode : public ExpressionNode {
    ExpressionNode* expr;
    std::string op;
    ModuleCompiler& compiler;

public:
    UnaryNode(ExpressionNode* expr, std::string op, ModuleCompiler& compiler);

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Type* get_type(llvm::LLVMContext& context) const override;
};

class BlockNode : public ASTNode {
public:
    BlockNode(std::vector<ASTNode*> nodes, ModuleCompiler& compiler);

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;

private:
    std::vector<ASTNode*> nodes;
    ModuleCompiler& compiler;
};

class FunctionNode : public ASTNode {
public:
    struct Parameter {
        std::string name;
        KType* type;
    };

    FunctionNode(const std::string& name, std::vector<Parameter> args, KType* ret_type, ASTNode* body,
                 ModuleCompiler& compiler);

    [[nodiscard]] std::string to_string() const override;
    llvm::Value* gen() override;

private:
    [[nodiscard]] std::vector<llvm::Type*> get_arg_types() const;

    std::string name;
    std::vector<Parameter> args;
    KType* ret_type;
    ASTNode* body;
    ModuleCompiler& compiler;
};