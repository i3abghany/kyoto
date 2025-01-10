#pragma once

#include <stdint.h>
#include <string>

#include "kyoto/AST/ASTNode.h"

class ModuleCompiler;
class KType;
class PrimitiveType;
namespace llvm {
class LLVMContext;
class Type;
class Value;
} // namespace llvm

class ExpressionNode : public ASTNode {
public:
    virtual ~ExpressionNode() = default;
    [[nodiscard]] virtual std::string to_string() const = 0;
    [[nodiscard]] virtual llvm::Value* gen() = 0;
    [[nodiscard]] virtual llvm::Type* gen_type(llvm::LLVMContext& context) const = 0;
    [[nodiscard]] virtual KType* get_ktype() const { return nullptr; }
    [[nodiscard]] virtual llvm::Value* trivial_gen() { return nullptr; }
    [[nodiscard]] virtual bool is_trivially_evaluable() const { return false; }
    [[nodiscard]] virtual llvm::Value* gen_ptr() const { return nullptr; }

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

class IdentifierExpressionNode : public ExpressionNode {
public:
    IdentifierExpressionNode(std::string name, ModuleCompiler& compiler);
    ~IdentifierExpressionNode() override = default;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Value* gen_ptr() const override;
    [[nodiscard]] llvm::Type* gen_type(llvm::LLVMContext& context) const override;
    [[nodiscard]] KType* get_ktype() const override;

    [[nodiscard]] const std::string& get_name() const { return name; }

private:
    std::string name;
    ModuleCompiler& compiler;
};

class AssignmentNode : public ExpressionNode {
public:
    AssignmentNode(ExpressionNode* assignee, ExpressionNode* expr, ModuleCompiler& compiler);
    ~AssignmentNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Type* gen_type(llvm::LLVMContext& context) const override;
    [[nodiscard]] llvm::Value* trivial_gen() override;
    [[nodiscard]] KType* get_ktype() const override;
    [[nodiscard]] bool is_trivially_evaluable() const override;

private:
    [[nodiscard]] llvm::Value* gen_deref_assignment();

private:
    ExpressionNode* assignee;
    ExpressionNode* expr;
    ModuleCompiler& compiler;
};

class ReturnStatementNode : public ASTNode {
    ExpressionNode* expr;
    ModuleCompiler& compiler;

public:
    ReturnStatementNode(ExpressionNode* expr, ModuleCompiler& compiler);
    ~ReturnStatementNode() override;

    [[nodiscard]] std::string to_string() const override;
    llvm::Value* gen() override;
};

class UnaryNode : public ExpressionNode {
public:
    enum class UnaryOp {
        Negate,
        Positive,
        PrefixIncrement,
        PrefixDecrement,
        AddressOf,
        Dereference,
    };

    UnaryNode(ExpressionNode* expr, UnaryOp op, ModuleCompiler& compiler);
    ~UnaryNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Value* gen_ptr() const override;
    [[nodiscard]] llvm::Type* gen_type(llvm::LLVMContext& context) const override;
    [[nodiscard]] KType* get_ktype() const override;
    [[nodiscard]] llvm::Value* trivial_gen() override;
    [[nodiscard]] bool is_trivially_evaluable() const override;

    [[nodiscard]] ExpressionNode* get_expr() const { return expr; }
    [[nodiscard]] UnaryOp get_op() const { return op; }

private:
    std::string op_to_string() const;
    bool simple_op() const;

    llvm::Value* gen_prefix_increment();
    llvm::Value* gen_prefix_decrement();

private:
    mutable KType* type;
    ExpressionNode* expr;
    UnaryOp op;
    ModuleCompiler& compiler;
};