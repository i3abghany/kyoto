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

    static void check_boolean_promotion(PrimitiveType* expr_ktype, PrimitiveType* target_type,
                                        const std::string& target_name);

    static void check_int_range_fit(int64_t val, PrimitiveType* target_type, ModuleCompiler& compiler,
                                    const std::string& expr, const std::string& target_name);

    static llvm::Value* dynamic_integer_conversion(llvm::Value* expr_val, PrimitiveType* expr_ktype,
                                                   PrimitiveType* target_type, ModuleCompiler& compiler);

    static llvm::Value* promoted_trivially_gen(ExpressionNode* expr, ModuleCompiler& compiler, KType* target_ktype,
                                               const std::string& target_name);

    static llvm::Value* handle_integer_conversion(ExpressionNode* expr, KType* target_type, ModuleCompiler& compiler,
                                                  const std::string& what, const std::string& target_name = "");
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

private:
    ExpressionNode* expr;
    ModuleCompiler& compiler;
};

class IdentifierExpressionNode : public ExpressionNode {
    std::string name;
    ModuleCompiler& compiler;

public:
    IdentifierExpressionNode(std::string name, ModuleCompiler& compiler);
    ~IdentifierExpressionNode() = default;

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

class AssignmentNode : public ExpressionNode {
public:
    AssignmentNode(std::string name, ExpressionNode* expr, ModuleCompiler& compiler);
    ~AssignmentNode();

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Type* get_type(llvm::LLVMContext& context) const override;
    [[nodiscard]] llvm::Value* trivial_gen() override;
    [[nodiscard]] bool is_trivially_evaluable() const override;

private:
    std::string name;
    ExpressionNode* expr;
    ModuleCompiler& compiler;
};

class ReturnStatementNode : public ASTNode {
    ExpressionNode* expr;
    ModuleCompiler& compiler;

public:
    ReturnStatementNode(ExpressionNode* expr, ModuleCompiler& compiler);
    ~ReturnStatementNode();

    [[nodiscard]] std::string to_string() const override;
    llvm::Value* gen() override;
};

class UnaryNode : public ExpressionNode {
    ExpressionNode* expr;
    std::string op;
    ModuleCompiler& compiler;

public:
    UnaryNode(ExpressionNode* expr, std::string op, ModuleCompiler& compiler);
    ~UnaryNode();

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Type* get_type(llvm::LLVMContext& context) const override;
};

class BlockNode : public ASTNode {
public:
    BlockNode(std::vector<ASTNode*> nodes, ModuleCompiler& compiler);
    ~BlockNode();

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
    ~FunctionNode();

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