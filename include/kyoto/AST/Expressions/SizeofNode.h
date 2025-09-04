#pragma once

#include <string>

#include "kyoto/AST/Expressions/ExpressionNode.h"

class KType;
class ModuleCompiler;

class SizeofNode : public ExpressionNode {
public:
    enum class OperandType { Expression, Type };

    SizeofNode(ExpressionNode* expr, ModuleCompiler& compiler);

    SizeofNode(KType* type, ModuleCompiler& compiler);

    ~SizeofNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Value* gen_ptr() const override;
    [[nodiscard]] llvm::Type* gen_type() const override;
    [[nodiscard]] KType* get_ktype() const override;
    [[nodiscard]] llvm::Value* trivial_gen() override;
    [[nodiscard]] bool is_trivially_evaluable() const override;
    [[nodiscard]] std::vector<ASTNode*> get_children() const override;

    [[nodiscard]] OperandType get_operand_type() const { return operand_type; }
    [[nodiscard]] ExpressionNode* get_expr() const { return expr; }
    [[nodiscard]] KType* get_type() const { return type; }

private:
    OperandType operand_type;
    ExpressionNode* expr;
    KType* type;
    ModuleCompiler& compiler;
};
