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
public:
    enum class UnaryOp {
        Negate,
        Positive,
        PrefixIncrement,
        PrefixDecrement,
    };

    UnaryNode(ExpressionNode* expr, UnaryOp op, ModuleCompiler& compiler);
    ~UnaryNode();

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Type* get_type(llvm::LLVMContext& context) const override;
    [[nodiscard]] llvm::Value* trivial_gen() override;
    [[nodiscard]] bool is_trivially_evaluable() const override;

private:
    std::string op_to_string() const;

private:
    ExpressionNode* expr;
    UnaryOp op;
    ModuleCompiler& compiler;
};