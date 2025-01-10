#pragma once

#include <fmt/core.h>
#include <iostream>

#include "llvm/IR/Type.h"

#include "kyoto/KType.h"

class ModuleCompiler;
class ExpressionNode;

namespace llvm {
class LLVMContext;
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

class ProgramNode final : public ASTNode {
    std::vector<ASTNode*> nodes;
    ModuleCompiler& compiler;

public:
    ProgramNode(std::vector<ASTNode*> nodes, ModuleCompiler& compiler);
    ~ProgramNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
};

class ExpressionStatementNode final : public ASTNode {
public:
    ExpressionStatementNode(ExpressionNode* expr, ModuleCompiler& compiler);
    ~ExpressionStatementNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;

    [[nodiscard]] ExpressionNode* get_expr() const { return expr; }

private:
    ExpressionNode* expr;
};

class BlockNode final : public ASTNode {
public:
    BlockNode(std::vector<ASTNode*> nodes, ModuleCompiler& compiler);
    ~BlockNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;

    [[nodiscard]] const std::vector<ASTNode*>& get_nodes() const { return nodes; }

private:
    std::vector<ASTNode*> nodes;
    ModuleCompiler& compiler;
};

class FunctionNode final : public ASTNode {
public:
    struct Parameter {
        std::string name;
        KType* type;
    };

    FunctionNode(std::string name, std::vector<Parameter> args, bool varargs, KType* ret_type, ASTNode* body,
                 ModuleCompiler& compiler);
    ~FunctionNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;

    [[nodiscard]] const std::vector<Parameter>& get_params() const { return args; }
    [[nodiscard]] std::string get_name() const { return name; }

    [[nodiscard]] KType* get_ret_type() const { return ret_type; }

private:
    [[nodiscard]] std::vector<llvm::Type*> get_arg_types() const;

    std::string name;
    std::vector<Parameter> args;
    bool varargs;
    KType* ret_type;
    ASTNode* body;
    ModuleCompiler& compiler;
};