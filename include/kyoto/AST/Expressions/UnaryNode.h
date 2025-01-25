#pragma once

#include <string>
#include <vector>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/Expressions/ExpressionNode.h"

class KType;
class ModuleCompiler;
namespace llvm {
class Value;
} // namespace llvm

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
    [[nodiscard]] llvm::Type* gen_type() const override;
    [[nodiscard]] KType* get_ktype() const override;
    [[nodiscard]] llvm::Value* trivial_gen() override;
    [[nodiscard]] bool is_trivially_evaluable() const override;
    [[nodiscard]] std::vector<ASTNode*> get_children() const override { return { expr }; }

    [[nodiscard]] ExpressionNode* get_expr() const { return expr; }
    [[nodiscard]] UnaryOp get_op() const { return op; }

private:
    std::string op_to_string() const;
    bool simple_op() const;

    llvm::Value* gen_prefix_increment() const;
    llvm::Value* gen_prefix_decrement() const;

private:
    mutable KType* type;
    ExpressionNode* expr;
    UnaryOp op;
    ModuleCompiler& compiler;
};