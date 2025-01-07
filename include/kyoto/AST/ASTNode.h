#pragma once

#include <fmt/core.h>
#include <iostream>

#include "llvm/IR/Type.h"

#include "kyoto/KType.h"

class ModuleCompiler;
class ExpressionNode;

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

class ProgramNode : public ASTNode {
    std::vector<ASTNode*> nodes;
    ModuleCompiler& compiler;

public:
    ProgramNode(std::vector<ASTNode*> nodes, ModuleCompiler& compiler);
    ~ProgramNode();

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
};

class ExpressionStatementNode : public ASTNode {
public:
    ExpressionStatementNode(ExpressionNode* expr, ModuleCompiler& compiler);
    ~ExpressionStatementNode();

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;

    [[nodiscard]] ExpressionNode* get_expr() const { return expr; }

private:
    ExpressionNode* expr;
    ModuleCompiler& compiler;
};

class BlockNode : public ASTNode {
public:
    BlockNode(std::vector<ASTNode*> nodes, ModuleCompiler& compiler);
    ~BlockNode();

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;

    [[nodiscard]] const std::vector<ASTNode*>& get_nodes() const { return nodes; }

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

    FunctionNode(const std::string& name, std::vector<Parameter> args, bool varargs, KType* ret_type, ASTNode* body,
                 ModuleCompiler& compiler);
    ~FunctionNode();

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;

    [[nodiscard]] const std::vector<Parameter>& get_params() const { return args; }
    [[nodiscard]] std::string get_name() const { return name; }

private:
    [[nodiscard]] std::vector<llvm::Type*> get_arg_types() const;

    std::string name;
    std::vector<Parameter> args;
    bool varargs;
    KType* ret_type;
    ASTNode* body;
    ModuleCompiler& compiler;
};