#pragma once

#include <string>
#include <vector>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/Expressions/ExpressionNode.h"

class FunctionNode;
class FunctionType;
class ModuleCompiler;

namespace llvm {
class Value;
}

class AnonymousFunctionNode final : public ExpressionNode {
public:
    AnonymousFunctionNode(FunctionNode* function, FunctionType* type, ModuleCompiler& compiler);
    ~AnonymousFunctionNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Type* gen_type() const override;
    [[nodiscard]] KType* get_ktype() const override;
    [[nodiscard]] std::vector<ASTNode*> get_children() const override;

private:
    FunctionNode* function;
    FunctionType* type;
    ModuleCompiler& compiler;
};
