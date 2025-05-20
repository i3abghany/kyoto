#pragma once

#include <string>
#include <vector>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/Expressions/ExpressionNode.h"
#include "kyoto/AST/Expressions/FunctionCallNode.h"

class KType;
class ModuleCompiler;

class NewNode : public ExpressionNode {
public:
    NewNode(KType* type, FunctionCall* constructor_call, ModuleCompiler& compiler);

    ~NewNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Value* gen_ptr() const override;
    [[nodiscard]] llvm::Type* gen_type() const override;
    [[nodiscard]] KType* get_ktype() const override;
    [[nodiscard]] llvm::Value* trivial_gen() override;
    [[nodiscard]] bool is_trivially_evaluable() const override;

    [[nodiscard]] std::vector<ASTNode*> get_children() const override { return { constructor_call }; }

    [[nodiscard]] KType* get_type() const { return type; }
    [[nodiscard]] FunctionCall* get_constructor_call() const { return constructor_call; }

private:
    KType* type;
    KType* generated_type;
    FunctionCall* constructor_call;
    ModuleCompiler& compiler;
};