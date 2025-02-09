#pragma once

#include <string>
#include <utility>
#include <vector>

#include "kyoto/AST/Expressions/ExpressionNode.h"

class ModuleCompiler;
class KType;

class ArrayNode : public ExpressionNode {
public:
    ArrayNode(std::vector<ExpressionNode*> elements, KType* type, ModuleCompiler& compiler)
        : elements(std::move(elements))
        , type(type)
        , compiler(compiler)
    {
    }

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Type* gen_type() const override;
    [[nodiscard]] KType* get_ktype() const override { return type; }
    [[nodiscard]] std::vector<ASTNode*> get_children() const override;

    [[nodiscard]] const std::vector<ExpressionNode*>& get_elements() const { return elements; }

private:
    std::vector<ExpressionNode*> elements;
    KType* type;
    ModuleCompiler& compiler;
};