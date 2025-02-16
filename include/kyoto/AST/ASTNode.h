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
class Function;
}

class ASTNode {
public:
    virtual ~ASTNode() = default;
    [[nodiscard]] virtual std::string to_string() const = 0;
    virtual llvm::Value* gen() = 0;
    virtual std::vector<ASTNode*> get_children() const = 0;

    template <typename T> bool is() const { return dynamic_cast<const T*>(this) != nullptr; }
    template <typename T> T* as() { return dynamic_cast<T*>(this); }
    template <typename T> const T* as() const { return dynamic_cast<const T*>(this); }

    static llvm::Type* get_llvm_type(const KType* type, ModuleCompiler& compiler);
};

class ProgramNode final : public ASTNode {
public:
    ProgramNode(std::vector<ASTNode*> nodes, ModuleCompiler& compiler);
    ~ProgramNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;

    [[nodiscard]] std::vector<ASTNode*> get_children() const override { return nodes; }

private:
    std::vector<ASTNode*> nodes;
    ModuleCompiler& compiler;
};

class ExpressionStatementNode final : public ASTNode {
public:
    ExpressionStatementNode(ExpressionNode* expr, ModuleCompiler& compiler);
    ~ExpressionStatementNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;

    [[nodiscard]] ExpressionNode* get_expr() const { return expr; }

    [[nodiscard]] std::vector<ASTNode*> get_children() const override;

private:
    ExpressionNode* expr;
};

class BlockNode final : public ASTNode {
public:
    BlockNode(std::vector<ASTNode*> nodes, ModuleCompiler& compiler);
    ~BlockNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;

    [[nodiscard]] std::vector<ASTNode*> get_children() const override { return nodes; }

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

    FunctionNode(std::string name, std::vector<Parameter> args, bool varargs, KType* ret_type, ASTNode* body,
                 ModuleCompiler& compiler);
    ~FunctionNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Function* gen_prototype();

    [[nodiscard]] const std::vector<Parameter>& get_params() const { return args; }
    [[nodiscard]] std::string get_name() const { return name; }
    [[nodiscard]] std::vector<ASTNode*> get_children() const override { return { body }; }

    [[nodiscard]] KType* get_ret_type() const { return ret_type; }

    void insert_arg(const Parameter& arg, size_t index);

    void set_body(ASTNode* body) { this->body = body; }

private:
    [[nodiscard]] std::vector<llvm::Type*> get_arg_types() const;

    std::string name;
    std::vector<Parameter> args;
    bool varargs;
    KType* ret_type;
    ASTNode* body;
    ModuleCompiler& compiler;
};